#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <stdbool.h>
#include <getopt.h>
#include <time.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../include/stb_image_write.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../include/stb_image.h"

#define DS_ARRAY_IMPLEMENT
#include "../include/ds_array.h"

// time measure
#define TIME_START gettimeofday(&my__start, NULL);
#define TIME_END gettimeofday(&my__end, NULL); printf("耗时: %f 秒\n", my__end.tv_sec - my__start.tv_sec + (my__end.tv_usec - my__start.tv_usec) / 1000000.0);
struct timeval my__start, my__end; 

// TODO: safe random
#define get_random(n) (rand() % n)

// opea debug mode
// #define KMEANS_DEBUG_MODE
#ifdef KMEANS_DEBUG_MODE
    #define DEBUG(...) printf(__VA_ARGS__)
#else
    #define DEBUG(...) ((void)0)  
#endif

// some define data area

#define KMEAN_MAX_ITERATIONS 10
#define KMEAN_EPS 0.000001
#define KMEAN_MAX_DIST 1e9

// struct area

typedef struct{
    uint8_t r, g, b;
} rgb;

typedef struct {
    double r, g, b;
} RgbCenter;

typedef struct{
    int width;
    int height;
    int size;
    rgb* g;
}Picture;

// define function

#define RGB_TO_CENTER(p) ((RgbCenter){ (double)(p).r, (double)(p).g, (double)(p).b })

// 声明防止顺序问题
static inline Picture open_picture(char *filename);
static inline int ker_select(double *dist_list, uint64_t sum_squqre);
static inline void kmean_calc_dist(Picture pic, RgbCenter* ker_list, double *dist_list, uint64_t *sum_squqre);
static inline double kmean_update_ker(Picture pic, RgbCenter* ker_list, double * dist_list, RgbCenter* dist_sum_list, int *count_list);

#define RGB_DIST_CENTER(x, y) ((x.r - y.r) * (x.r - y.r) + (x.g - y.g) * (x.g - y.g) + (x.b - y.b) * (x.b - y.b))

static inline Picture open_picture(char *filename){
    int x, y, n;
    unsigned char *data = stbi_load(filename, &x, &y, &n, 0);

    Picture pic = {0};

    if(data == NULL){
        fprintf(stderr, "Failed to load image: %s\n", filename);
        exit(1);
    }
    
    if (n != 3){
        stbi_image_free(data);
        fprintf(stderr, "Image must have 3 channels (RGB), got %d\n", n);
        exit(1);
    }

    pic.size = x * y;
    pic.width = x; 
    pic.height = y;
    pic.g = (rgb*)data;

    return pic;
}

static inline int ker_select(double *dist_list, uint64_t sum_squqre){
    int r = get_random(sum_squqre);

    int dist_list_len = array_len(dist_list);
    for (int i = 0; i < dist_list_len; i ++){
        r -= dist_list[i];
        if (r <= 0) return i;
    }

    return dist_list_len;
}

static inline void kmean_calc_dist(Picture pic, RgbCenter* ker_list, double *dist_list, uint64_t *sum_squqre){
    int ker_list_len = array_len(ker_list);
    for (int i = 0; i < pic.size; i ++){
        dist_list[i] = KMEAN_MAX_DIST;
        for (int j = 0; j < ker_list_len; j ++){
            double new_d = RGB_DIST_CENTER(pic.g[i], ker_list[j]);
            if (dist_list[i] > new_d){
                dist_list[i] = new_d;
            }
        }
        *sum_squqre += dist_list[i];
    }
}

static inline double kmean_update_ker(Picture pic, RgbCenter* ker_list, double * dist_list, RgbCenter* dist_sum_list, int *count_list){

    memset(count_list, 0, array_size(count_list));
    
    int ker_list_len = array_len(ker_list);
    for (int i = 0; i < ker_list_len; i ++){
        dist_list[i] = KMEAN_MAX_DIST;
        dist_sum_list[i].r = 0;
        dist_sum_list[i].g = 0;
        dist_sum_list[i].b = 0;
    }
    
    for (int i = 0; i < pic.size; i ++){
        int target_ker = 0;
        // 1. 找到最近的ker
        for (int j = 0; j < ker_list_len; j ++){
            double new_d = RGB_DIST_CENTER(pic.g[i], ker_list[j]);
            if (dist_list[i] > new_d){
                dist_list[i] = new_d;
                target_ker = j;
            }
        }

        // 2. 更新其中的sum 和 count
        dist_sum_list[target_ker].r += pic.g[i].r; 
        dist_sum_list[target_ker].g += pic.g[i].g; 
        dist_sum_list[target_ker].b += pic.g[i].b; 
        count_list[target_ker] ++;
    }

    // 3.更新其中ker
    double mov = 0;

    // TODO: temp test
    printf("old ker list : \n");
    for (int i = 0 ; i < ker_list_len; i ++){
        printf("[r g b] %.2e %.2e %.2e\n", ker_list[i].r, ker_list[i].g, ker_list[i].b);
    }

    for (int i = 0; i < ker_list_len; i ++){
        if (!count_list[i]) continue;

        mov += fabs(ker_list[i].r - dist_sum_list[i].r / count_list[i]);
        mov += fabs(ker_list[i].g - dist_sum_list[i].g / count_list[i]);
        mov += fabs(ker_list[i].b - dist_sum_list[i].b / count_list[i]);

        ker_list[i].r = dist_sum_list[i].r / count_list[i];
        ker_list[i].g = dist_sum_list[i].g / count_list[i];
        ker_list[i].b = dist_sum_list[i].b / count_list[i];
    }

    printf("new ker list : \n");
    for (int i = 0 ; i < ker_list_len; i ++){
        printf("[r g b] %.2e %.2e %.2e\n", ker_list[i].r, ker_list[i].g, ker_list[i].b);
    }
    printf("\n");

    return mov;
}

/*
 * 注意如果需要 array_push 需要修改 ker_list 为 ker_list_ptr
 * returns :
 *  - ker_list
 */
static inline void kmean_main(Picture pic, RgbCenter* ker_list){
    // 1. 第一个随机选择 = array_len(ker_list);

    double * dist_list = array_strict_init(sizeof(double) * pic.size);
    int ker_list_len = array_len(ker_list);

    DEBUG("[ker selcct] first ker\n");
    ker_list[0] = RGB_TO_CENTER(pic.g[get_random(pic.size)]);

    if (!dist_list || !ker_list) goto clean;

    // 2. 剩下的核心
    uint64_t sum_squqre = 0;

    for (int i = 1; i < ker_list_len ; i ++){
        DEBUG("[ker select] round %d\n", i);
        kmean_calc_dist(pic, ker_list, dist_list, &sum_squqre); 
        int pos = ker_select(dist_list, sum_squqre);
        
        sum_squqre = 0;
        ker_list[i] = RGB_TO_CENTER(pic.g[pos]);
    }

    // 3 不断更新
    int round = KMEAN_MAX_ITERATIONS;
    
    RgbCenter *dist_sum_list = array_strict_init(sizeof(RgbCenter) * ker_list_len);
    int *count_list = array_strict_init(sizeof(int) * ker_list_len);

    // 检查是否分配
    if (dist_sum_list == NULL || count_list == NULL) goto clean;

    while (round --){
        double mov = kmean_update_ker(pic, ker_list, dist_list, dist_sum_list, count_list);
        DEBUG("[round] %d the mov is %.3e\n", round, mov);
        if (fabs(mov) <= KMEAN_EPS) break;
    }    
    
clean:
    if (dist_list) array_free(dist_list);
    if (dist_sum_list) array_free(dist_sum_list);
    if (count_list) array_free(count_list);
    DEBUG("kmean_main clean up is over\n");
}

static inline uint8_t * generate_kmean_data(Picture pic, RgbCenter* ker_list){
    uint8_t * res = (uint8_t *)malloc(sizeof(uint8_t) * pic.size * 3);

    int ker_list_len = array_len(ker_list);
    for (int i = 0; i < pic.size; i ++){
        int target_ker = 0;
        double dist_i = KMEAN_MAX_DIST;

        for (int j = 0; j < ker_list_len; j ++){
            double new_d = RGB_DIST_CENTER(pic.g[i], ker_list[j]);
            if (dist_i > new_d){
                dist_i = new_d;
                target_ker = j;
            }
        }
    
        res[i * 3] = ker_list[target_ker].r;
        res[i * 3 + 1] = ker_list[target_ker].g;
        res[i * 3 + 2] = ker_list[target_ker].b;
    }

    return res;
}
    
int main(int argc, char *argv[]){
    TIME_START;
    srand(time(0));
    
    // 参数变量
    char *image_path = NULL;
    char *output_path = NULL;  // 新增：输出文件路径
    int ker_number = 0;
    int opt;
    
    // 1. 解析命令行参数
    while ((opt = getopt(argc, argv, "i:c:o:")) != -1) {  // 添加 o: 选项
        switch (opt) {
            case 'i':
                image_path = optarg;
                break;
            case 'c':
                ker_number = atoi(optarg);
                if (ker_number <= 0) {
                    fprintf(stderr, "错误: 聚类数量必须是正整数\n");
                    return 1;
                }
                break;
            case 'o':  // 新增：输出文件选项
                output_path = optarg;
                break;
            case '?':
                // 未知选项
                if (optopt == 'i' || optopt == 'c' || optopt == 'o') {  // 添加 o
                    fprintf(stderr, "错误: 选项 -%c 需要参数\n", optopt);
                } else {
                    fprintf(stderr, "错误: 未知选项 -%c\n", optopt);
                }
                return 1;
            default:
                return 1;
        }
    }
    
    if (image_path == NULL) {
        fprintf(stderr, "错误: 必须指定输入图片路径 (-i)\n");
        return 1;
    }
    
    if (ker_number == 0) {
        fprintf(stderr, "错误: 必须指定聚类数量 (-c)\n");
        return 1;
    }
    
    if (output_path == NULL) {
        output_path = "output.png";
        printf("提示: 未指定输出文件，使用默认路径: %s\n", output_path);
    }else{
        int len = strlen(output_path);
        if (len <= 5){
            fprintf(stderr, "错误: 输出图片路径错误 (-c)\n");
        }else{
            if (strcmp(output_path + len - 4, ".png") != 0){
                fprintf(stderr, "错误: 输出图片只支持 .png (-c)\n");
            }
        }
    }

    // 2. 生成 ker_list
    Picture pic = open_picture(image_path);
    RgbCenter *ker_list = array_strict_init(ker_number * sizeof(RgbCenter));
    kmean_main(pic, ker_list);

    // 3. 生成图片
    uint8_t *pic_data = generate_kmean_data(pic, ker_list);   
    stbi_write_png(output_path, pic.width, pic.height, 3, pic_data, pic.width * 3);

    stbi_image_free(pic.g);
    array_free(ker_list);

    return 0;
}