// =========================================================================
// Visual Studio用 セキュリティ警告強制解除 ＆ ヘッダーインクルード
// =========================================================================
#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable:4996) 

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>//commit
#include <string.h>
#include <windows.h> // Sleep関数と色変更（SetConsoleTextAttribute）に必要

// --- 定数定義 ---
#define PRODUCT_NUM 5    // 基本設計書仕様の「5商品」
#define COIN_TYPES 5     // 対応する金種の数 (10, 50, 100, 500, 1000)

// --- 構造体定義 ---
typedef struct {
    int id;
    char name[20];
    int price;
    int stock;
} Product;

typedef struct {
    int value;
    int count;
} CoinStock;

// --- グローバル変数（基本設計書の既定値に準拠） ---
static Product products[PRODUCT_NUM] = {
    {1, "お茶",     110, 20}, // 初期在庫を20個に変更（テストしやすいよう正の数にしています）
    {2, "コーラ",   140, 20},
    {3, "コーヒー", 200, 20},
    {4, "オレンジ", 120, 20},
    {5, "紅茶",     180, 20}
};

// 金銭管理データ（0=10円, 1=50円, 2=100円, 3=500円, 4=1000円）
static CoinStock change_stock[COIN_TYPES] = {
    {10,   20},  // 0: 10円（初期値20枚）
    {50,   20},  // 1: 50円（初期値20枚）
    {100,  20},  // 2: 100円（初期値20枚）
    {500,  20},  // 3: 500円（初期値20枚）
    {1000, 0}    // 4: 1000円（初期値0枚）
};

int total_sales = 0;              // 累計売上額
int current_inserted = 0;         // 現在の入金合計額
int inserted_counts[COIN_TYPES] = { 0 }; // 今回のセッションで投入された金種の枚数

// --- 関数プロトタイプ宣言 ---
void set_color(int color);
void display_menu(void);
int can_make_change(int change_amount);
void refund(int change_amount);
bool update_and_save_data(int product_idx, int price);
void load_data_from_file(void);

// ==========================================
// 文字色変更関数 (Windowsコンソール用)
// 7:標準白, 11:水色(購入可能), 12:赤(売切・エラー)
// ==========================================
void set_color(int color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

// ==========================================
// 画面制御・描画処理（設計書完全準拠メニュー）
// ==========================================
void display_menu(void) {
    printf("************ 自動販売機 シミュレーション ソフトウェア ************\n");
    printf("金銭投入後商品を選択してください。");

    if (current_inserted > 0) {
        set_color(11);
        printf("  入金%d円", current_inserted);
        set_color(7);
    }
    printf("\n\n");

    printf("+------------+------------+------------+------------+------------+\n");
    printf("|  A.%s    |  B.%s  | C.%s | D.%s |  E.%s    |\n",
        products[0].name, products[1].name, products[2].name, products[3].name, products[4].name);
    printf("+------------+------------+------------+------------+------------+\n");

    printf("|");
    for (int i = 0; i < PRODUCT_NUM; i++) {
        if (products[i].stock <= 0) {
            printf("    ");
            set_color(12);
            printf("売切");
            set_color(7);
            printf("    |");
        }
        else {
            if (current_inserted >= products[i].price) {
                printf("    ");
                set_color(11);
                printf("%d", products[i].price);
                set_color(7);
                printf("     |");
            }
            else {
                printf("    %d      |", products[i].price);
            }
        }
    }
    printf("\n+------------+------------+------------+------------+------------+\n\n");

    printf("お金を入れてください。1:10円 2:50円 3:100円 4:500円 5:1000円\n");
}

// ==========================================
// 釣銭計算判定：高金種優先でシミュレーション
// 戻り値: 1 = お釣りがぴったり出せる（OK）, 0 = 釣銭切れ（NG）
// ==========================================
int can_make_change(int change_amount) {
    int temp_vault[COIN_TYPES];
    for (int i = 0; i < COIN_TYPES; i++) {
        temp_vault[i] = change_stock[i].count;
    }

    int coin_values[5] = { 1000, 500, 100, 50, 10 };
    int vault_indices[5] = { 4, 3, 2, 1, 0 };

    for (int i = 0; i < 5; i++) {
        int coin = coin_values[i];
        int v_idx = vault_indices[i];

        while (change_amount >= coin && temp_vault[v_idx] > 0) {
            change_amount -= coin;
            temp_vault[v_idx]--;
        }
    }
    return (change_amount == 0);
}

// ==========================================
// 実際の釣銭払い出し処理：金庫から硬貨を減算
// ==========================================
void refund(int change_amount) {
    int coin_values[5] = { 1000, 500, 100, 50, 10 };
    int vault_indices[5] = { 4, 3, 2, 1, 0 };

    for (int i = 0; i < 5; i++) {
        int coin = coin_values[i];
        int v_idx = vault_indices[i];

        while (change_amount >= coin && change_stock[v_idx].count > 0) {
            change_amount -= coin;
            change_stock[v_idx].count--;
        }
    }
}

// ==========================================
// データ更新・保存関数 (update_and_save_data)
// ==========================================
bool update_and_save_data(int product_idx, int price) {
    if (price > 0) {
        products[product_idx].stock--;
        total_sales += price;
    }

    FILE* fp_money = NULL;
    FILE* fp_product = NULL;

    if (fopen_s(&fp_money, "Money.csv", "w") != 0 || fp_money == NULL) {
        return false;
    }
    fprintf(fp_money, "累計売上額, 1000円紙幣, 500円硬貨, 100円硬貨, 50円硬貨, 10円硬貨\n");
    fprintf(fp_money, "%d円, %d枚, %d枚, %d枚, %d枚, %d枚\n",
        total_sales,
        change_stock[4].count,
        change_stock[3].count,
        change_stock[2].count,
        change_stock[1].count,
        change_stock[0].count);
    fclose(fp_money);

    if (fopen_s(&fp_product, "Product.csv", "w") != 0 || fp_product == NULL) {
        return false;
    }
    fprintf(fp_product, "商品ID, 商品名, 価格, 在庫数\n");
    for (int i = 0; i < PRODUCT_NUM; i++) {
        fprintf(fp_product, "%d, %s, %d円, %d個\n",
            products[i].id, products[i].name, products[i].price, products[i].stock);
    }
    fclose(fp_product);

    return true;
}

// ==========================================
// ファイルからデータを読み込む関数 (load_data_from_file)
// ==========================================
void load_data_from_file(void) {
    FILE* fp_product = NULL;
    FILE* fp_money = NULL;
    char line[150];

    if (fopen_s(&fp_product, "Product.csv", "r") == 0 && fp_product != NULL) {
        fgets(line, sizeof(line), fp_product);
        for (int i = 0; i < PRODUCT_NUM; i++) {
            if (fgets(line, sizeof(line), fp_product) != NULL) {
                // タイポ修正: sscansh_s -> sscanf_s
                sscanf_s(line, "%d, %[^,], %d円, %d個",
                    &products[i].id,
                    products[i].name, (unsigned int)sizeof(products[i].name),
                    &products[i].price,
                    &products[i].stock);
            }
        }
        fclose(fp_product);
    }

    if (fopen_s(&fp_money, "Money.csv", "r") == 0 && fp_money != NULL) {
        fgets(line, sizeof(line), fp_money);
        if (fgets(line, sizeof(line), fp_money) != NULL) {
            // タイポ修正: sscansh_s -> sscanf_s
            sscanf_s(line, "%d円, %d枚, %d枚, %d枚, %d枚, %d枚",
                &total_sales,
                &change_stock[4].count, &change_stock[3].count,
                &change_stock[2].count, &change_stock[1].count,
                &change_stock[0].count);
        }
        fclose(fp_money);
    }
}

// =========================================================================
// メイン制御ルーチン (main) - エラー完全仕分け ＆ ピンポイント釣銭切れブロック
// =========================================================================
int main(void) {
    char input[10];
    char error_msg[120] = "";
    char success_msg1[100] = "";
    char success_msg2[100] = "";
    int show_success = 0;

    // 起動時にファイルから最新データを復元
    load_data_from_file();

    while (true) {
        system("cls"); // 画面クリア（UI制御）
        display_menu();

        int can_buy_any = 0;
        int min_price = 9999;
        for (int i = 0; i < PRODUCT_NUM; i++) {
            if (products[i].stock > 0) {
                if (products[i].price < min_price) min_price = products[i].price;
                if (current_inserted >= products[i].price) can_buy_any = 1;
            }
        }

        if (can_buy_any && strlen(error_msg) == 0 && !show_success) {
            printf("商品購入可能です。\n");
        }

        // 赤文字で仕分けされたエラーメッセージを表示
        if (strlen(error_msg) > 0) {
            set_color(12);
            printf("%s\n", error_msg);
            set_color(7);
            error_msg[0] = '\0';
        }

        // 商品購入成功時の処理
        if (show_success) {
            set_color(11);
            printf("%s\n", success_msg1);
            if (strlen(success_msg2) > 0) {
                printf("%s\n", success_msg2);
            }
            set_color(7);
            show_success = 0;

            Sleep(10000); // 10秒待機

            if (current_inserted < min_price) {
                current_inserted = 0;
                memset(inserted_counts, 0, sizeof(inserted_counts));
            }
            continue;
        }

        printf(">> ");
        if (scanf_s("%9s", input, (unsigned int)sizeof(input)) != 1) {
            continue;
        }

        // 🌟 入力形式の厳密なチェック（チェック①：混ぜこぜ入力を弾く）
        int len = (int)strlen(input);
        if (len > 1) {
            strcpy_s(error_msg, sizeof(error_msg), "【入力エラー】文字と数字が混ざっているか、フォーマット違反のため認識できません。");
            int c;
            while ((c = getchar()) != '\n' && c != EOF); // バッファクリア
            continue;
        }

        // システムの終了＆返金処理
        if (strcmp(input, "9") == 0) {
            if (current_inserted > 0) {
                set_color(11);
                printf("\n%d円を返金します。お受け取りください。\n", current_inserted);
                set_color(7);
                refund(current_inserted);
                Sleep(10000);
            }
            else {
                printf("\nシステムを終了します。\n");
            }
            update_and_save_data(0, 0);
            break;
        }

        // お金の投入処理
        if (input[0] >= '1' && input[0] <= '5') {
            int idx = input[0] - '1';
            int limit = (idx == 4) ? 2 : 20;

            if (inserted_counts[idx] >= limit) {
                if (idx == 4) {
                    sprintf_s(error_msg, sizeof(error_msg), "紙幣の投入枚数が上限(%d枚)を超えています。", limit);
                }
                else {
                    sprintf_s(error_msg, sizeof(error_msg), "%d円硬貨の投入枚数が上限(%d枚)を超えています。", change_stock[idx].value, limit);
                }
            }
            else {
                current_inserted += change_stock[idx].value;
                inserted_counts[idx]++;
                change_stock[idx].count++;
            }
            continue;
        }

        // 商品の選択処理
        if ((input[0] >= 'A' && input[0] <= 'E') || (input[0] >= 'a' && input[0] <= 'e')) {
            int item_idx = -1;
            if (input[0] == 'A' || input[0] == 'a') item_idx = 0;
            if (input[0] == 'B' || input[0] == 'b') item_idx = 1;
            if (input[0] == 'C' || input[0] == 'c') item_idx = 2;
            if (input[0] == 'D' || input[0] == 'd') item_idx = 3;
            if (input[0] == 'E' || input[0] == 'e') item_idx = 4;

            if (products[item_idx].stock <= 0) {
                strcpy_s(error_msg, sizeof(error_msg), "売切れ商品です。他の商品を選択してください。");
                continue;
            }

            if (current_inserted < products[item_idx].price) {
                strcpy_s(error_msg, sizeof(error_msg), "お金が足りません。");
                continue;
            }

            int change_needed = current_inserted - products[item_idx].price;

            // 🌟【最重要チェック】物理的にお釣りが出せるか金庫の枚数をシミュレーションしてブロック
            if (can_make_change(change_needed) == 0) {
                // どの硬貨がピンポイントで足りないのかを判定してメッセージを出し分ける
                if (change_needed % 50 != 0 && change_stock[0].count == 0) {
                    strcpy_s(error_msg, sizeof(error_msg), "【釣銭切れ】10円硬貨が不足しているため、このお札・硬貨では購入できません。");
                }
                else if (change_needed >= 50 && change_needed < 100 && change_stock[1].count == 0 && change_stock[0].count < 5) {
                    strcpy_s(error_msg, sizeof(error_msg), "【釣銭切れ】50円硬貨が不足しているため、このお札・硬貨では購入できません。");
                }
                else if (change_needed >= 100 && change_stock[2].count == 0 && change_stock[1].count == 0 && change_stock[0].count == 0) {
                    strcpy_s(error_msg, sizeof(error_msg), "【釣銭切れ】金庫の硬貨が不足しているため、このお札・硬貨では購入できません。");
                }
                else {
                    strcpy_s(error_msg, sizeof(error_msg), "【釣銭切れ】お釣り用の硬貨（10円・50円・100円等）が不足しているため購入できません。");
                }
                continue;
            }

            // 安全確認が取れたら減算処理を実行
            refund(change_needed);
            update_and_save_data(item_idx, products[item_idx].price);
            current_inserted = change_needed;
            memset(inserted_counts, 0, sizeof(inserted_counts));

            sprintf_s(success_msg1, sizeof(success_msg1), "%sの購入、ありがとうございました。", products[item_idx].name);
            if (change_needed > 0) {
                sprintf_s(success_msg2, sizeof(success_msg2), "お釣りは%d円です。%sをお受け取りください。", change_needed, products[item_idx].name);
            }
            else {
                sprintf_s(success_msg2, sizeof(success_msg2), "%sをお受け取りください。", products[item_idx].name);
            }
            show_success = 1;
            continue;
        }

        // 🌟 チェック②：1文字だけど規定外の文字（記号や無効なアルファベット）を弾く
        strcpy_s(error_msg, sizeof(error_msg), "【無効な文字】メニューにない記号やアルファベットは認識できません。");
    }

    return 0;
}
