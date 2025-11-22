#include "mongoose.h"
#include "sqlite3.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static sqlite3 *g_db = NULL;

/****************************************************
  JSON エスケープ（危険文字から守る）
****************************************************/
static void json_escape(const char *src, char *dst, size_t dst_sz) {
    size_t j = 0;
    for (size_t i = 0; src[i] && j + 2 < dst_sz; i++) {
        unsigned char c = (unsigned char)src[i];
        if (c == '\"' || c == '\\') {
            if (j + 2 >= dst_sz) break;
            dst[j++] = '\\';
            dst[j++] = c;
        } else if (c < 0x20) {
            if (j + 6 >= dst_sz) break;
            j += snprintf(dst + j, dst_sz - j, "\\u%04x", c);
        } else {
            dst[j++] = c;
        }
    }
    dst[j] = '\0';
}

/****************************************************
  SQL の LIKE 用に入力値をサニタイズ
****************************************************/
static void sanitize_like(const char *src, char *dst, size_t dst_sz) {
    size_t j = 0;
    for (size_t i = 0; src[i] && j + 2 < dst_sz; i++) {
        char c = src[i];
        if (c == '%' || c == '_' || c == '\\') {
            if (j + 2 >= dst_sz) break;
            dst[j++] = '\\';
            dst[j++] = c;
        } else {
            dst[j++] = c;
        }
    }
    dst[j] = '\0';
}

/****************************************************
  JSON バッファ追加
****************************************************/
static void append_str(char **buf, size_t *len, size_t *cap, const char *src) {
    size_t add = strlen(src);
    if (*len + add + 1 > *cap) {
        size_t newcap = (*cap == 0) ? 4096 : *cap * 2;
        while (newcap < *len + add + 1) newcap *= 2;
        char *nb = (char *)realloc(*buf, newcap);
        if (!nb) return;
        *buf = nb;
        *cap = newcap;
    }
    memcpy(*buf + *len, src, add);
    *len += add;
    (*buf)[*len] = '\0';
}

/****************************************************
  Angular形式の日付をDB形式に変換
    strlen(src) == 10 - 入力が10文字（YYYY-MM-DD 形式）かチェック

    snprintf(dst, size, "%.4s%.2s%.2s", src, src +5, src +8) - 文字列を分解して結合
    %.4s → src の先頭4文字（年：YYYY）
    %.2s → src +5 の2文字（月：MM）
    %.2s → src +8 の2文字（日：DD）
    dst[0] = '\0' - 形式が正しくない場合は空文字列にする
    ＜参考＞ char *src は、文字列の先頭アドレスを指している
    メモリアドレス: 1000  1001  1002  1003  1004  1005  1006  1007  1008  1009  1010
    文字:           '2'   '0'   '2'   '4'   '-'   '0'   '1'   '-'   '2'   '3'   '\0'
    位置:            0     1     2     3     4     5     6     7     8     9     10
                    ↑                             ↑
                    src                         src+5
****************************************************/
static void date_to_db_format(const char *src, char *dst, size_t size) {
    // "YYYY-MM-DD" 形式を "YYYYMMDD" 形式に変換
    if (strlen(src) == 10) {
        snprintf(dst, size, "%.4s%.2s%.2s", src, src +5, src +8);
    } else {
        dst[0] = '\0';
    }
}

/****************************************************
  /api/search ハンドラ
****************************************************/
static void handle_api_search(struct mg_connection *c, struct mg_http_message *hm) {

    printf("handle_api_search() called\n");

    char customerCode[128] = "";
    char customerName[256] = "";
    char customerKana[256] = "";
    char productName[256] = "";
    char productCode[128] = "";
    char startDate[32] = "";
    char endDate[32] = "";
    char dbStart[16] = "";
    char dbEnd[16] = "";

    // クエリパラメータを取得
    // &hm->query - HTTPリクエストのクエリ文字列（URL の ? 以降の部分）
    // 例
    // "customerCode" - 取得したいパラメータの名前
    // customerCode - 取得した値を格納するバッファ
    // sizeof(customerCode) - バッファのサイズ
    mg_http_get_var(&hm->query, "customerCode", customerCode, sizeof(customerCode));
    mg_http_get_var(&hm->query, "customerName", customerName, sizeof(customerName));
    mg_http_get_var(&hm->query, "customerKana", customerKana, sizeof(customerKana));
    mg_http_get_var(&hm->query, "productName",  productName,  sizeof(productName));
    mg_http_get_var(&hm->query, "productCode",  productCode,  sizeof(productCode));
    mg_http_get_var(&hm->query, "startDate",    startDate,    sizeof(startDate));
    mg_http_get_var(&hm->query, "endDate",      endDate,      sizeof(endDate));

    const char *base_sql =
        "SELECT "
        "sale_date, customer_code, customer_name, customer_short, customer_kana, "
        "product_name, product_code, amount, quantity, unit_name, torihiki_name "
        "FROM sales WHERE 1=1";

    char sql[2000];
    strcpy(sql, base_sql);

    // 動的に SQL を構築
    // strcat とは、文字列を連結する関数
    // 例えば、customerCode が指定されていれば、
    // " AND customer_code = ?" が SQL に追加される =? はプレースホルダ
    // 後で sqlite3_bind_text() で値をバインドする
    // なお、customer_code は完全一致検索、他は部分一致検索（LIKE）としている
    if (customerCode[0])  strcat(sql, " AND customer_code = ?");
    if (customerName[0])  strcat(sql, " AND customer_name LIKE ?");
    if (customerKana[0])  strcat(sql, " AND customer_kana LIKE ?");
    if (productName[0])   strcat(sql, " AND product_name LIKE ?");
    if (productCode[0])
        strcat(sql, " AND product_code LIKE ? ESCAPE '\\'"); // ← CHANGED
    if (startDate[0] && endDate[0]) {
    strcat(sql, " AND sale_date BETWEEN ? AND ?");
    }
    strcat(sql, " ORDER BY sale_date DESC");

    printf("SQL => %s\n", sql);

    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        mg_http_reply(c, 500, "", "DB prepare error");
        printf("DB prepare error: %s\n", sqlite3_errmsg(g_db));
        return;
    }

    char likeBuf[512];
    int idx = 1;

    // プレースホルダに値をバインド
    // sqlite3_bind_text() は、指定したプレースホルダに文字列をバインドする関数
    // idx はプレースホルダの位置（1から始まる）
    // -1 は文字列の長さを自動的に計算することを意味する
    // SQLインジェクション対策として、直接 SQL に値を埋め込むのではなく、
    // プレースホルダを使って値をバインドしている
    if (customerCode[0]) {
        sqlite3_bind_text(stmt, idx++, customerCode, -1, SQLITE_TRANSIENT);
    }
    if (customerName[0]) {
        sanitize_like(customerName, likeBuf, sizeof(likeBuf));
        char wrapped[600];
        snprintf(wrapped, sizeof(wrapped), "%%%s%%", likeBuf);
        sqlite3_bind_text(stmt, idx++, wrapped, -1, SQLITE_TRANSIENT);
    }
    if (customerKana[0]) {
        sanitize_like(customerKana, likeBuf, sizeof(likeBuf));
        char wrapped[600];
        snprintf(wrapped, sizeof(wrapped), "%%%s%%", likeBuf);
        sqlite3_bind_text(stmt, idx++, wrapped, -1, SQLITE_TRANSIENT);
    }
    if (productName[0]) {
        sanitize_like(productName, likeBuf, sizeof(likeBuf));
        char wrapped[600];
        snprintf(wrapped, sizeof(wrapped), "%%%s%%", likeBuf);
        sqlite3_bind_text(stmt, idx++, wrapped, -1, SQLITE_TRANSIENT);
    }

    if (productCode[0]) {   // ← CHANGED
        sanitize_like(productCode, likeBuf, sizeof(likeBuf));
        char wrapped[600];
        snprintf(wrapped, sizeof(wrapped), "%%%s%%", likeBuf);
        sqlite3_bind_text(stmt, idx++, wrapped, -1, SQLITE_TRANSIENT);
    }

    if (startDate[0] && endDate[0]) {
        // Angular形式の日付を DB 形式に変換
        date_to_db_format(startDate, dbStart, sizeof(dbStart));
        date_to_db_format(endDate, dbEnd, sizeof(dbEnd));

        sqlite3_bind_text(stmt, idx++, dbStart, -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, idx++, dbEnd, -1, SQLITE_TRANSIENT);
    }

    char *json = NULL;
    size_t len = 0, cap = 0;
    append_str(&json, &len, &cap, "[");

    int first = 1;
    while (sqlite3_step(stmt) == SQLITE_ROW) {

        char esc_sale_date[128], esc_customer_code[128], esc_customer_name[256];
        char esc_customer_short[256], esc_customer_kana[256], esc_product_name[256];
        char esc_product_code[128], esc_unit_name[128], esc_torihiki_name[128];

        json_escape((const char*)sqlite3_column_text(stmt, 0), esc_sale_date, sizeof(esc_sale_date));
        json_escape((const char*)sqlite3_column_text(stmt, 1), esc_customer_code, sizeof(esc_customer_code));
        json_escape((const char*)sqlite3_column_text(stmt, 2), esc_customer_name, sizeof(esc_customer_name));
        json_escape((const char*)sqlite3_column_text(stmt, 3), esc_customer_short, sizeof(esc_customer_short));
        json_escape((const char*)sqlite3_column_text(stmt, 4), esc_customer_kana, sizeof(esc_customer_kana));
        json_escape((const char*)sqlite3_column_text(stmt, 5), esc_product_name, sizeof(esc_product_name));
        json_escape((const char*)sqlite3_column_text(stmt, 6), esc_product_code, sizeof(esc_product_code));
        json_escape((const char*)sqlite3_column_text(stmt, 9), esc_unit_name, sizeof(esc_unit_name));
        json_escape((const char*)sqlite3_column_text(stmt, 10), esc_torihiki_name, sizeof(esc_torihiki_name));

        int amount   = sqlite3_column_int(stmt, 7);
        int quantity = sqlite3_column_int(stmt, 8);

        char row[2048];
        snprintf(row, sizeof(row),
            "%s{\"sale_date\":\"%s\",\"customer_code\":\"%s\",\"customer_name\":\"%s\","
            "\"customer_short\":\"%s\",\"customer_kana\":\"%s\",\"product_name\":\"%s\","
            "\"product_code\":\"%s\",\"amount\":%d,\"quantity\":%d,"
            "\"unit_name\":\"%s\",\"torihiki_name\":\"%s\"}",
            first ? "" : ",",
            esc_sale_date, esc_customer_code, esc_customer_name,
            esc_customer_short, esc_customer_kana, esc_product_name,
            esc_product_code, amount, quantity,
            esc_unit_name, esc_torihiki_name
        );

        append_str(&json, &len, &cap, row);
        first = 0;
    }

    append_str(&json, &len, &cap, "]");
    sqlite3_finalize(stmt);

    mg_http_reply(c, 200,
        "Content-Type: application/json\r\n"
        "Access-Control-Allow-Origin: *\r\n",
        "%s", json
    );

    free(json);
}


/****************************************************
  Mongoose イベントハンドラ
****************************************************/
static void fn(struct mg_connection *c, int ev, void *ev_data, void *fn_data) {

    printf("EVENT: %d\n", ev);

    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *)ev_data;

        printf("HTTP message received\n");
        printf("Method: %.*s  URI: %.*s\n",
            (int)hm->method.len, hm->method.ptr,
            (int)hm->uri.len, hm->uri.ptr);

        if (mg_http_match_uri(hm, "/api/search")) {
            printf("MATCHED /api/search\n");
            handle_api_search(c, hm);
        } else {
            mg_http_reply(c, 404, "", "Not found");
        }
    }
}

/****************************************************
  メイン処理
****************************************************/
int main(void) {

    if (sqlite3_open("sales.db", &g_db) != SQLITE_OK) {
        printf("ERROR: Cannot open sales.db\n");
        return 1;
    }

    printf("sales.db opened successfully\n");

    struct mg_mgr mgr;
    mg_mgr_init(&mgr);

    struct mg_connection *c =
        mg_http_listen(&mgr, "http://0.0.0.0:8081", fn, NULL);

    if (!c) {
        printf("ERROR: cannot listen on port 8081\n");
        return 1;
    }

    printf("Listening on http://localhost:8081\n");

    for (;;) {
        mg_mgr_poll(&mgr, 1000);
    }

    sqlite3_close(g_db);
    mg_mgr_free(&mgr);
    return 0;
}
