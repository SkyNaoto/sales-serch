import csv
import sqlite3

# ★★★ CSV のファイル名（あなたの環境用） ★★★
SALES_CSV = "売上リスト20241228.csv"
CUSTOMER_CSV = "得意先マスター20250222.csv"

# ★★★ 出力する SQLite DB ★★★
DB_NAME = "sales.db"

# SQLite を新規作成
conn = sqlite3.connect(DB_NAME)
cur = conn.cursor()

# 既存テーブル削除（再生成用）
cur.execute("DROP TABLE IF EXISTS sales")

# sales テーブル生成（UTF-8）
cur.execute("""
CREATE TABLE sales (
    sale_date TEXT,
    customer_code TEXT,
    customer_name TEXT,
    customer_short TEXT,
    customer_kana TEXT,
    product_name TEXT,
    product_code TEXT,
    amount INTEGER,
    quantity INTEGER,
    unit_name TEXT,
    torihiki_name TEXT
);
""")

# -------------------------------
# 得意先マスターの読み込み（Shift-JIS）
# -------------------------------
customer_map = {}  # key: 得意先コード, value: dict()

with open(CUSTOMER_CSV, "r", encoding="cp932") as f:
    reader = csv.DictReader(f)
    for row in reader:
        code = row.get("得意先コード", "").strip()
        if not code:
            continue

        customer_map[code] = {
            "customer_name": row.get("得意先名", "").strip(),
            "customer_short": row.get("略名", "").strip(),
            "customer_kana": row.get("検索カナ名", "").strip()
        }

print(f"得意先マスター読み込み完了（{len(customer_map)} 件）")


# -------------------------------
# 売上リストの読み込み（Shift-JIS）
# -------------------------------
with open(SALES_CSV, "r", encoding="cp932") as f:
    reader = csv.DictReader(f)

    count = 0
    for row in reader:
        sale_date = row.get("伝票日付", "").strip()
        customer_code = row.get("得意先コード", "").strip()

        # 得意先情報の補完
        cinfo = customer_map.get(customer_code, {
            "customer_name": "",
            "customer_short": "",
            "customer_kana": ""
        })

        product_name = row.get("商品名", "").strip()
        product_code = row.get("商品コード", "").strip()
        amount = row.get("売上金額", "0").strip()
        quantity = row.get("取引数量", "0").strip()
        unit_name = row.get("単位名称", "").strip()
        torihiki_name = row.get("取引名", "").strip()

        cur.execute("""
        INSERT INTO sales (
            sale_date, customer_code, customer_name, customer_short, customer_kana,
            product_name, product_code, amount, quantity, unit_name, torihiki_name
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
        """, (
            sale_date,
            customer_code,
            cinfo["customer_name"],
            cinfo["customer_short"],
            cinfo["customer_kana"],
            product_name,
            product_code,
            int(amount) if amount.isdigit() else 0,
            int(quantity) if quantity.isdigit() else 0,
            unit_name,
            torihiki_name
        ))

        count += 1
        if count % 10000 == 0:
            print(f"{count} 件処理...")

conn.commit()
conn.close()

print(f"\n完了！ SQLite DB \"{DB_NAME}\" に {count} 件の売上データを登録しました。")
