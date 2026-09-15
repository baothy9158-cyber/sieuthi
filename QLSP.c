#include <stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>
struct product
{
    char ten[20];
    int gia;
    int soLuong;
    int ID;
};
struct product *products = NULL;
int count = 0;
int capacity = 0;

/* ================== KHAI BAO TRUOC (PROTOTYPE) ================== */
int ensureCapacity(struct product **products, int *capacity, int count);
int saveProducts(const char *filename);
void rebuildBST(void); // dung trong phan Stack/Undo va Queue ben duoi

/* ================== STACK (NGAN XEP) - CHUC NANG UNDO ================== */
#define MAX_UNDO 100

typedef enum
{
    ACTION_ADD,     // thao tac them san pham
    ACTION_DELETE,  // thao tac xoa san pham
    ACTION_EDIT     // thao tac sua san pham
} ActionType;

typedef struct
{
    ActionType type;
    struct product data; // du lieu can thiet de hoan tac
    int index;           // vi tri trong mang products luc thao tac xay ra
} Action;

Action undoStack[MAX_UNDO];
int undoTop = -1; // -1 nghia la stack rong (LIFO: dinh stack la phan tu them sau cung)

// Day (push) mot thao tac vao dinh stack
void pushUndo(ActionType type, struct product data, int index)
{
    if (undoTop >= MAX_UNDO - 1)
    {
        // Stack day: bo phan tu cu nhat (day toan bo xuong 1 vi tri)
        for (int i = 0; i < MAX_UNDO - 1; i++)
            undoStack[i] = undoStack[i + 1];
        undoTop--;
    }
    undoTop++;
    undoStack[undoTop].type = type;
    undoStack[undoTop].data = data;
    undoStack[undoTop].index = index;
}

// Lay (pop) thao tac o dinh stack ra, tra ve 1 neu thanh cong
int popUndo(Action *action)
{
    if (undoTop < 0)
        return 0;
    *action = undoStack[undoTop];
    undoTop--;
    return 1;
}

// Hoan tac thao tac gan nhat (LIFO)
void undo()
{
    Action action;
    if (!popUndo(&action))
    {
        printf("Khong co thao tac nao de undo\n");
        return;
    }

    switch (action.type)
    {
    case ACTION_ADD:
        // Hoan tac THEM: xoa san pham vua them (nam o cuoi mang)
        if (count > 0)
        {
            count--;
            saveProducts("products.txt");
            rebuildBST();
            printf("Da hoan tac THEM: xoa san pham ID=%d\n", action.data.ID);
        }
        break;

    case ACTION_DELETE:
        // Hoan tac XOA: chen lai san pham vao dung vi tri cu
        if (!ensureCapacity(&products, &capacity, count))
        {
            printf("Khong the cap phat bo nho de undo\n");
            return;
        }
        for (int i = count; i > action.index; i--)
        {
            products[i] = products[i - 1];
        }
        products[action.index] = action.data;
        count++;
        saveProducts("products.txt");
        rebuildBST();
        printf("Da hoan tac XOA: khoi phuc san pham ID=%d\n", action.data.ID);
        break;

    case ACTION_EDIT:
        // Hoan tac SUA: khoi phuc lai du lieu cu tai vi tri do
        if (action.index >= 0 && action.index < count)
        {
            products[action.index] = action.data;
            saveProducts("products.txt");
            rebuildBST();
            printf("Da hoan tac SUA: khoi phuc san pham ID=%d ve gia tri cu\n", action.data.ID);
        }
        else
        {
            printf("Khong the hoan tac, vi tri khong hop le\n");
        }
        break;
    }
}
/* ================== HET PHAN STACK - UNDO ================== */

/* ================== TREE (CAY NHI PHAN TIM KIEM - BST) - TIM KIEM NHANH ================== */
struct TreeNode
{
    int ID;
    int index; // vi tri hien tai cua san pham nay trong mang products
    struct TreeNode *left;
    struct TreeNode *right;
};
struct TreeNode *root = NULL;

struct TreeNode* insertBST(struct TreeNode *node, int id, int index)
{
    if (node == NULL)
    {
        struct TreeNode *newNode = (struct TreeNode*)malloc(sizeof(struct TreeNode));
        newNode->ID = id;
        newNode->index = index;
        newNode->left = NULL;
        newNode->right = NULL;
        return newNode;
    }
    if (id < node->ID)
        node->left = insertBST(node->left, id, index);
    else if (id > node->ID)
        node->right = insertBST(node->right, id, index);
    return node;
}

void freeBST(struct TreeNode *node)
{
    if (node == NULL)
        return;
    freeBST(node->left);
    freeBST(node->right);
    free(node);
}

// Xay dung lai toan bo cay BST tu mang products hien tai.
// Duoc goi lai moi khi mang thay doi (them/xoa/sap xep/undo/doc file)
// de dam bao chi so (index) luu trong cay luon dung.
void rebuildBST(void)
{
    freeBST(root);
    root = NULL;
    for (int i = 0; i < count; i++)
        root = insertBST(root, products[i].ID, i);
}

struct TreeNode* searchBST(struct TreeNode *node, int id)
{
    if (node == NULL)
        return NULL;
    if (node->ID == id)
        return node;
    if (id < node->ID)
        return searchBST(node->left, id);
    return searchBST(node->right, id);
}

void timKiemTheoBST(int id)
{
    struct TreeNode *result = searchBST(root, id);
    if (result == NULL)
    {
        printf("(BST) Khong tim thay san pham co ID %d\n", id);
        return;
    }
    int idx = result->index;
    printf("(BST) Tim thay: ID=%d, Ten=%s, Gia=%d, So luong=%d\n",
           products[idx].ID, products[idx].ten, products[idx].gia, products[idx].soLuong);
}
/* ================== HET PHAN TREE - BST ================== */

int ensureCapacity(struct product **products, int *capacity, int count)
{
    if (count < *capacity)
        return 1;

    int newCapacity = (*capacity == 0) ? 2 : *capacity * 2;
    struct product *temporary = realloc(
        *products, newCapacity * sizeof(struct product));

    if (temporary == NULL)
        return 0;

    *products = temporary;
    *capacity = newCapacity;
    return 1;
}

int idExists(const struct product products[], int count, int id)
{
    for (int i = 0; i < count; i++)
    {
        if (products[i].ID == id)
            return 1;
    }

    return 0;
}
void saveHistoryadded(const char *filename)
{
    FILE *file = fopen(filename, "a");
    if (file == NULL)
    {
        printf("Khong the mo file de ghi lich su\n");
        return;
    }

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    fprintf(file,"-------------------------------\n");
    fprintf(file, "Lich su them san pham:[%02d:%02d]Added\n",
            t->tm_hour, t->tm_min);

    fprintf(file, "%d %s %d %d\n",
            products[count - 1].ID,
            products[count - 1].ten,
            products[count - 1].gia,
            products[count - 1].soLuong);

    fclose(file);
}
void saveHistorydeleted(const char *filename, struct product deletedProduct)
{
    FILE *file = fopen(filename, "a");
    if (file == NULL)
    {
        printf("Khong the mo file de ghi lich su\n");
        return;
    }

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    fprintf(file,"-------------------------------\n");
    fprintf(file, "Lich su xoa san pham:[%02d:%02d]Deleted\n",
            t->tm_hour, t->tm_min);

    fprintf(file, "%d %s %d %d\n",
            deletedProduct.ID,
            deletedProduct.ten,
            deletedProduct.gia,
            deletedProduct.soLuong);

    fclose(file);
}
void saveHistoryEdit(const char *filename, struct product editedProduct)
{
    FILE *file = fopen(filename, "a");
    if (file == NULL)
    {
        printf("Khong the mo file de ghi lich su\n");
        return;
    }
    
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    fprintf(file,"-------------------------------\n");
    fprintf(file, "Lich su sua san pham:[%02d:%02d]Edited\n",
            t->tm_hour, t->tm_min);
    fprintf(file, "%d %s %d %d\n",
            editedProduct.ID,
            editedProduct.ten,
            editedProduct.gia,
            editedProduct.soLuong);
    fclose(file);
}
int saveProducts(const char *filename)
{
    FILE *file = fopen(filename, "w");

    if (file == NULL)
    {
        printf("Khong the mo file de ghi\n");
        return 0;
    }

    for (int i = 0; i < count; i++)
    {
        fprintf(file, "%d %s %d %d\n",
                products[i].ID,
                products[i].ten,
                products[i].gia,
                products[i].soLuong);
    }

    fclose(file);
    return 1;
}
int nhapProduct(struct product **products, int *count, int *capacity)
{
    int newID;

    if (!ensureCapacity(products, capacity, *count))
    {
        printf("Khong the cap phat bo nho\n");
        return 0;
    }

    printf("Nhap ID san pham: ");
    scanf("%d", &newID);

    if (newID < 0 || newID > 1000)
    {
        printf("ID khong hop le\n");
        return 0;
    }
    if (idExists(*products, *count, newID))
    {
        printf("ID da ton tai\n");
        return 0;
    }

    (*products)[*count].ID = newID;
    printf("Nhap ten san pham: ");
    scanf("%19s", (*products)[*count].ten);
    printf("Nhap gia san pham: ");
    scanf("%d", &(*products)[*count].gia);
    printf("Nhap so luong san pham: ");
    scanf("%d", &(*products)[*count].soLuong);
    (*count)++;
    if (saveProducts("products.txt"))
    {
        printf("Da luu san pham vao file\n");
    }
    else
    {
        printf("Khong the luu san pham vao file\n");
    }
    // Day thao tac THEM vao stack undo
    pushUndo(ACTION_ADD, (*products)[*count - 1], *count - 1);
    rebuildBST();
    return 1;
}
void xuatProduct(const struct product *products, int count)
{
    if (count == 0)
    {
        printf("Danh sach trong!\n");
        return;
    }

    printf("\n+------+----------------------+----------+----------+\n");
    printf("| %-4s | %-20s | %-8s | %-8s |\n", "ID", "Ten san pham", "Gia", "So luong");
    printf("+------+----------------------+----------+----------+\n");

    for (int i = 0; i < count; i++)
    {
        printf("| %-4d | %-20s | %-8d | %-8d |\n",
               products[i].ID,
               products[i].ten,
               products[i].gia,
               products[i].soLuong);
    }
    printf("+------+----------------------+----------+----------+\n");
}

int loadProducts(const char *filename)
{
    FILE *file = fopen(filename, "r");

    if (file == NULL)
    {
        printf("Khong the mo file de doc\n");
        return 0;
    }
    count = 0;
    while (fscanf(file, "%d %19s %d %d",
                  &products[count].ID,
                  products[count].ten,
                  &products[count].gia,
                  &products[count].soLuong) == 4)
    {
        printf("Da doc san pham: ID=%d, Ten=%s, Gia=%d, So luong=%d\n",
               products[count].ID,
               products[count].ten,
               products[count].gia,
               products[count].soLuong);
        count++;
        if (!ensureCapacity(&products, &capacity, count))
        {
            printf("Khong the cap phat bo nho\n");
            fclose(file);
            return 0;
        }
    }

    fclose(file);
    rebuildBST();
    return 1;
}
void xoaproduct(char *keyword)//chức năng xóa sản phẩm theo ID va ten
{
    int id = atoi(keyword);
    int index = -1;
    for (int i = 0; i < count; i++)
    {
        if (products[i].ID == id || strcmp(products[i].ten, keyword) == 0)
        {
            index = i;
            break;
        }
    }

    if (index == -1)
    {
        printf("San pham voi ID %d khong ton tai\n", id);
        return;
    }
    struct product deletedProduct = products[index];
    for (int i = index; i < count - 1; i++)
    {
        products[i] = products[i + 1];
    }
    count--;
    saveProducts("products.txt");
    saveHistorydeleted("history.txt", deletedProduct);
    // Day thao tac XOA vao stack undo (luu lai vi tri va du lieu de khoi phuc)
    pushUndo(ACTION_DELETE, deletedProduct, index);
    rebuildBST();
    printf("Da xoa san pham voi ID %d\n", id);
}
void searchProduct(char *keyword)
{
    int gia = atoi(keyword);
    int id = atoi(keyword);
    int found = 0;
    for (int i = 0; i < count; i++)
    {
        if (products[i].ID == id || strcmp(products[i].ten, keyword) == 0 || products[i].gia == gia)
        {
            printf("San pham tim thay: ID=%d, Ten=%s, Gia=%d, So luong=%d\n",products[i].ID,products[i].ten,products[i].gia,products[i].soLuong);
            found = 1; 
        }
    }

    if (!found)
    {
        printf("Khong tim thay san pham voi ID %d hoac ten %s\n", id, keyword);
    }
}
void editProduct(char *keyword)
{
    int id = atoi(keyword);
    int index = -1;
    for (int i = 0; i < count; i++)
    {
        if (products[i].ID == id || strcmp(products[i].ten, keyword) == 0)
        {
            index = i;
            break;
        }
    }

    if (index == -1)
    {
        printf("San pham voi ID %d khong ton tai\n", id);
        return;
    }

    // Luu lai du lieu cu truoc khi sua, de con undo duoc
    struct product oldProduct = products[index];

    printf("Nhap ten san pham moi: ");
    scanf("%19s", products[index].ten);
    printf("Nhap gia san pham moi: ");
    scanf("%d", &products[index].gia);
    printf("Nhap so luong san pham moi: ");
    scanf("%d", &products[index].soLuong);
    saveProducts("products.txt");
    saveHistoryEdit("history.txt", products[index]);
    // Day thao tac SUA vao stack undo (luu du lieu CU de khoi phuc)
    pushUndo(ACTION_EDIT, oldProduct, index);
    rebuildBST();
    printf("Da cap nhat thong tin san pham %s\n", products[index].ten);
}
void insertionSortByIDtangdan(struct product arr[], int count) {
    for (int i = 1; i < count; i++) {
        struct product key = arr[i];
        int j = i - 1;
        while (j >= 0 && arr[j].ID > key.ID) {
            arr[j + 1] = arr[j];
            j = j - 1;
        }
        arr[j + 1] = key;
    }
    printf("Da sap xep danh sach theo ID tang dan.\n");
}
void insertionSortByIDgiamdan(struct product arr[], int count) {
    for (int i = 1; i < count; i++) {
        struct product key = arr[i];
        int j = i - 1;
        while (j >= 0 && arr[j].ID < key.ID) {
            arr[j + 1] = arr[j];
            j = j - 1;
        }
        arr[j + 1] = key;
    }
    printf("Da sap xep danh sach theo ID giam dan.\n");
}
void insertionSortByTenAtoZ(struct product arr[], int count) {
    for (int i = 1; i < count; i++) {
        struct product key = arr[i];
        int j = i - 1;
        while (j >= 0 && strcmp(arr[j].ten, key.ten) > 0) {
            arr[j + 1] = arr[j];
            j = j - 1;
        }
        arr[j + 1] = key;
    }
    printf("Da sap xep danh sach theo ten tu A den Z.\n");
}
void insertionSortByTenZtoA(struct product arr[], int count) {
    for (int i = 1; i < count; i++) {
        struct product key = arr[i];
        int j = i - 1;
        while (j >= 0 && strcmp(arr[j].ten, key.ten) < 0) {
            arr[j + 1] = arr[j];
            j = j - 1;
        }
        arr[j + 1] = key;
    }
    printf("Da sap xep danh sach theo ten tu Z den A.\n");
}
void insertionSortByGiaTangDan(struct product arr[], int count) {
    for (int i = 1; i < count; i++) {
        struct product key = arr[i];
        int j = i - 1;
        while (j >= 0 && arr[j].gia > key.gia) {
            arr[j + 1] = arr[j];
            j = j - 1;
        }
        arr[j + 1] = key;
    }
    printf("Da sap xep danh sach theo gia tang dan.\n");
}
void insertionSortByGiaGiamDan(struct product arr[], int count) {
    for (int i = 1; i < count; i++) {
        struct product key = arr[i];
        int j = i - 1;
        while (j >= 0 && arr[j].gia < key.gia) {
            arr[j + 1] = arr[j];
            j = j - 1;
        }
        arr[j + 1] = key;
    }
    printf("Da sap xep danh sach theo gia giam dan.\n");
}
void statistics(struct product arr[],int count){
    int totalcost=0;
    for(int i=0;i<count;i++){
        totalcost+=arr[i].gia*arr[i].soLuong;
    }
    printf("Tong gia tri cua tat ca san pham: %d\n", totalcost);
    
}

/* ================== QUEUE (HANG DOI) - XU LY YEU CAU THEO FIFO ================== */
#define MAX_QUEUE 100

typedef enum
{
    REQ_ADD,    
    REQ_DELETE,
    REQ_EDIT    
} RequestType;

typedef struct
{
    RequestType type;
    struct product data;  // du lieu san pham (dung cho THEM va SUA)
    char keyword[20];     // ID/ten dung de tim (dung cho XOA va SUA)
} Request;

Request requestQueue[MAX_QUEUE];
int queueFront = 0; // vi tri dau hang doi (phan tu se duoc xu ly tiep theo)
int queueRear = 0;  // vi tri cuoi hang doi (noi them yeu cau moi vao)

int enqueueRequest(Request req)
{
    if (queueRear >= MAX_QUEUE)
    {
        printf("Hang doi da day, khong the them yeu cau moi!\n");
        return 0;
    }
    requestQueue[queueRear] = req;
    queueRear++;
    return 1;
}

int dequeueRequest(Request *req)
{
    if (queueFront == queueRear)
        return 0; // hang doi rong
    *req = requestQueue[queueFront];
    queueFront++;
    return 1;
}

// Them mot yeu cau moi vao cuoi hang doi (chua xu ly ngay, chi luu lai)
void themYeuCauVaoHangDoi()
{
    Request req;
    memset(&req, 0, sizeof(Request));

    printf("Chon loai yeu cau:\n");
    printf("1. Yeu cau THEM san pham\n");
    printf("2. Yeu cau XOA san pham\n");
    printf("3. Yeu cau SUA san pham\n");
    int loai;
    scanf("%d", &loai);

    switch (loai)
    {
    case 1:
        req.type = REQ_ADD;
        printf("Nhap ID san pham: ");
        scanf("%d", &req.data.ID);
        printf("Nhap ten san pham: ");
        scanf("%19s", req.data.ten);
        printf("Nhap gia san pham: ");
        scanf("%d", &req.data.gia);
        printf("Nhap so luong san pham: ");
        scanf("%d", &req.data.soLuong);
        break;
    case 2:
        req.type = REQ_DELETE;
        printf("Nhap ID hoac ten san pham can xoa: ");
        scanf("%19s", req.keyword);
        break;
    case 3:
        req.type = REQ_EDIT;
        printf("Nhap ID hoac ten san pham can sua: ");
        scanf("%19s", req.keyword);
        printf("Nhap ten moi: ");
        scanf("%19s", req.data.ten);
        printf("Nhap gia moi: ");
        scanf("%d", &req.data.gia);
        printf("Nhap so luong moi: ");
        scanf("%d", &req.data.soLuong);
        break;
    default:
        printf("Loai yeu cau khong hop le\n");
        return;
    }

    if (enqueueRequest(req))
        printf("Da them yeu cau vao hang doi. So yeu cau dang cho: %d\n", queueRear - queueFront);
}

// Xu ly toan bo hang doi theo dung thu tu FIFO (vao truoc, xu ly truoc)
void xuLyHangDoi()
{
    if (queueFront == queueRear)
    {
        printf("Hang doi dang trong, khong co yeu cau nao de xu ly\n");
        return;
    }

    printf("Bat dau xu ly hang doi theo FIFO...\n");
    Request req;
    while (dequeueRequest(&req))
    {
        switch (req.type)
        {
        case REQ_ADD:
            if (!ensureCapacity(&products, &capacity, count))
            {
                printf("Loi bo nho, bo qua yeu cau THEM ID=%d\n", req.data.ID);
                break;
            }
            if (idExists(products, count, req.data.ID))
            {
                printf("Yeu cau THEM ID=%d: ID da ton tai, bo qua\n", req.data.ID);
                break;
            }
            products[count] = req.data;
            count++;
            saveProducts("products.txt");
            pushUndo(ACTION_ADD, products[count - 1], count - 1);
            rebuildBST();
            printf("Da xu ly yeu cau THEM: ID=%d\n", req.data.ID);
            break;

        case REQ_DELETE:
            xoaproduct(req.keyword); // ham nay tu in thong bao va tu cap nhat BST/undo
            break;

        case REQ_EDIT:
        {
            int id = atoi(req.keyword);
            int index = -1;
            for (int i = 0; i < count; i++)
            {
                if (products[i].ID == id || strcmp(products[i].ten, req.keyword) == 0)
                {
                    index = i;
                    break;
                }
            }
            if (index == -1)
            {
                printf("Yeu cau SUA: khong tim thay san pham %s, bo qua\n", req.keyword);
                break;
            }
            struct product oldProduct = products[index];
            strcpy(products[index].ten, req.data.ten);
            products[index].gia = req.data.gia;
            products[index].soLuong = req.data.soLuong;
            saveProducts("products.txt");
            saveHistoryEdit("history.txt", products[index]);
            pushUndo(ACTION_EDIT, oldProduct, index);
            rebuildBST();
            printf("Da xu ly yeu cau SUA: ID=%d\n", products[index].ID);
            break;
        }
        }
    }
    printf("Da xu ly xong tat ca yeu cau trong hang doi.\n");
}
/* ================== HET PHAN QUEUE ================== */

int main(void)
{
    int select;
    int selectSort;
    char keyword[20];
    do
    {
        printf("\nChon chuc nang:\n");
        printf("1. Nhap san pham\n");
        printf("2. Xuat san pham\n");
        printf("3. Xoa san pham\n");
        printf("4. Doc san pham tu file\n");
        printf("5. Tim kiem san pham (tuyen tinh)\n");
        printf("6. Chinh sua san pham\n");
        printf("7. Sap xep san pham\n");
        printf("8. Undo thao tac gan nhat (them/xoa/sua)\n");
        printf("9. Them yeu cau vao hang doi (Queue)\n");
        printf("10. Xu ly hang doi theo FIFO\n");
        printf("11. Tim kiem nhanh theo ID bang cay BST\n");
        printf("0. Thoat\n");
        scanf("%d", &select);

        switch (select)
        {
        case 1:
            nhapProduct(&products, &count, &capacity);
            break;
        case 2:
            xuatProduct(products, count);
            break;
        case 3:
            printf("Nhap ID hoac ten san pham can xoa: ");
            scanf("%19s", keyword);
            xoaproduct(keyword);
            break;
        case 4:
            loadProducts("products.txt");
            break;
        case 5:
            printf("Nhap ID hoac ten san pham can tim: ");
            scanf("%19s", keyword);
            searchProduct(keyword);
            break;
        case 6:
            printf("Nhap ID hoac ten san pham can chinh sua: ");
            scanf("%19s", keyword);
            editProduct(keyword);
            break;
        case 7:
            printf("Chon chuc nang sap xep:\n");
            printf("1. Sap xep theo ID tang dan\n");
            printf("2. Sap xep theo ID giam dan\n");
            printf("3. Sap xep theo ten tu A den Z\n");
            printf("4. Sap xep theo ten tu Z den A\n");
            printf("5. Sap xep theo gia tang dan\n");
            printf("6. Sap xep theo gia giam dan\n");
            scanf("%d", &selectSort);
            switch (selectSort)
            {
            case 1:
                insertionSortByIDtangdan(products, count);
                break;
            case 2:
                insertionSortByIDgiamdan(products, count);
                break;
            case 3:
                insertionSortByTenAtoZ(products, count);
                break;
            case 4:
                insertionSortByTenZtoA(products, count);
                break;
            case 5:
                insertionSortByGiaTangDan(products, count);
                break;
            case 6:
                insertionSortByGiaGiamDan(products, count);
                break;
            default:
                printf("Chuc nang sap xep khong hop le\n");
            }
            xuatProduct(products, count);
            saveProducts("products.txt");
            rebuildBST(); // sap xep lam thay doi vi tri, phai xay lai cay BST
        break;
        case 8:
            undo();
            break;
        case 9:
            themYeuCauVaoHangDoi();
            break;
        case 10:
            xuLyHangDoi();
            break;
        case 11:
        {
            int idCanTim;
            printf("Nhap ID can tim (bang BST): ");
            scanf("%d", &idCanTim);
            timKiemTheoBST(idCanTim);
            break;
        }
        case 0:
            break;
        default:
            printf("Chuc nang khong hop le\n");
            break;
        }
    } while (select != 0);

    freeBST(root);
    free(products);
    return 0;
}