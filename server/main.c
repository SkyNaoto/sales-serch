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
  /api/search ハンドラ
****************************************************/
static void handle_api_search(struct mg_connection *c, struct mg_http_message *hm) {

    printf("handle_api_search() called\n");

    char customerCode[128] = "";
    char customerName[256] = "";
    char customerKana[256] = "";
    char productName[256] = "";
    char productCode[128] = "";

    mg_http_get_var(&hm->query, "customerCode", customerCode, sizeof(customerCode));
    mg_http_get_var(&hm->query, "customerName", customerName, sizeof(customerName));
    mg_http_get_var(&hm->query, "customerKana", customerKana, sizeof(customerKana));
    mg_http_get_var(&hm->query, "productName",  productName,  sizeof(productName));
    mg_http_get_var(&hm->query, "productCode",  productCode,  sizeof(productCode));

    const char *base_sql =
        "SELECT "
        "sale_date, customer_code, customer_name, customer_short, customer_kana, "
        "product_name, product_code, amount, quantity, unit_name, torihiki_name "
        "FROM sales WHERE 1=1";

    char sql[2000];
    strcpy(sql, base_sql);

    if (customerCode[0])  strcat(sql, " AND customer_code = ?");
    if (customerName[0])  strcat(sql, " AND customer_name LIKE ?");
    if (customerKana[0])  strcat(sql, " AND customer_kana LIKE ?");
    if (productName[0])   strcat(sql, " AND product_name LIKE ?");

    if (productCode[0])
        strcat(sql, " AND product_code LIKE ? ESCAPE '\\'"); // ← CHANGED

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
