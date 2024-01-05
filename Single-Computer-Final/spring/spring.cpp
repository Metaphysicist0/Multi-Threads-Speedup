#include <iostream>
#include<time.h>
#include<cmath>
#include<thread>
#include <windows.h>
#include <stack>
#include <immintrin.h>
#include <emmintrin.h>

#define MAX_THREADS 64
#define SUBDATANUM 2000000
#define DATANUM (SUBDATANUM*MAX_THREADS)
using namespace std;


float rawFloatData[DATANUM];
float sortmid[DATANUM];
float summid[MAX_THREADS];
float maxmid[MAX_THREADS];
HANDLE hThreads[MAX_THREADS];
HANDLE hSemaphores[MAX_THREADS];
HANDLE g_hHandle;

//包含一个静态成员变量permTable，这是一个256x8的二维数组，用于存储字节重排的映射表。
// 
//可以用于按特定顺序重新排列32位整数中的字节，以实现一些特定的操作或优化算法。

class PermLookupTable32Bit {
public:
	// shamelessly copied from https://github.com/damageboy/VxSort/blob/master/VxSort/BytePermutationTables.cs 
	// and modified to match my implementation
	static constexpr uint32_t permTable[256][8] = {
		{0, 1, 2, 3, 4, 5, 6, 7}, // 0b00000000 (0)|Left-PC: 8
		{1, 2, 3, 4, 5, 6, 7, 0}, // 0b00000001 (1)|Left-PC: 7    
		{0, 2, 3, 4, 5, 6, 7, 1}, // 0b00000010 (2)|Left-PC: 7    
		{2, 3, 4, 5, 6, 7, 0, 1}, // 0b00000011 (3)|Left-PC: 6
		{0, 1, 3, 4, 5, 6, 7, 2}, // 0b00000100 (4)|Left-PC: 7
		{1, 3, 4, 5, 6, 7, 0, 2}, // 0b00000101 (5)|Left-PC: 6
		{0, 3, 4, 5, 6, 7, 1, 2}, // 0b00000110 (6)|Left-PC: 6
		{3, 4, 5, 6, 7, 0, 1, 2}, // 0b00000111 (7)|Left-PC: 5
		{0, 1, 2, 4, 5, 6, 7, 3}, // 0b00001000 (8)|Left-PC: 7
		{1, 2, 4, 5, 6, 7, 0, 3}, // 0b00001001 (9)|Left-PC: 6
		{0, 2, 4, 5, 6, 7, 1, 3}, // 0b00001010 (10)|Left-PC: 6
		{2, 4, 5, 6, 7, 0, 1, 3}, // 0b00001011 (11)|Left-PC: 5
		{0, 1, 4, 5, 6, 7, 2, 3}, // 0b00001100 (12)|Left-PC: 6
		{1, 4, 5, 6, 7, 0, 2, 3}, // 0b00001101 (13)|Left-PC: 5
		{0, 4, 5, 6, 7, 1, 2, 3}, // 0b00001110 (14)|Left-PC: 5
		{4, 5, 6, 7, 0, 1, 2, 3}, // 0b00001111 (15)|Left-PC: 4
		{0, 1, 2, 3, 5, 6, 7, 4}, // 0b00010000 (16)|Left-PC: 7
		{1, 2, 3, 5, 6, 7, 0, 4}, // 0b00010001 (17)|Left-PC: 6
		{0, 2, 3, 5, 6, 7, 1, 4}, // 0b00010010 (18)|Left-PC: 6
		{2, 3, 5, 6, 7, 0, 1, 4}, // 0b00010011 (19)|Left-PC: 5
		{0, 1, 3, 5, 6, 7, 2, 4}, // 0b00010100 (20)|Left-PC: 6
		{1, 3, 5, 6, 7, 0, 2, 4}, // 0b00010101 (21)|Left-PC: 5
		{0, 3, 5, 6, 7, 1, 2, 4}, // 0b00010110 (22)|Left-PC: 5
		{3, 5, 6, 7, 0, 1, 2, 4}, // 0b00010111 (23)|Left-PC: 4
		{0, 1, 2, 5, 6, 7, 3, 4}, // 0b00011000 (24)|Left-PC: 6
		{1, 2, 5, 6, 7, 0, 3, 4}, // 0b00011001 (25)|Left-PC: 5
		{0, 2, 5, 6, 7, 1, 3, 4}, // 0b00011010 (26)|Left-PC: 5
		{2, 5, 6, 7, 0, 1, 3, 4}, // 0b00011011 (27)|Left-PC: 4
		{0, 1, 5, 6, 7, 2, 3, 4}, // 0b00011100 (28)|Left-PC: 5
		{1, 5, 6, 7, 0, 2, 3, 4}, // 0b00011101 (29)|Left-PC: 4
		{0, 5, 6, 7, 1, 2, 3, 4}, // 0b00011110 (30)|Left-PC: 4
		{5, 6, 7, 0, 1, 2, 3, 4}, // 0b00011111 (31)|Left-PC: 3
		{0, 1, 2, 3, 4, 6, 7, 5}, // 0b00100000 (32)|Left-PC: 7
		{1, 2, 3, 4, 6, 7, 0, 5}, // 0b00100001 (33)|Left-PC: 6
		{0, 2, 3, 4, 6, 7, 1, 5}, // 0b00100010 (34)|Left-PC: 6
		{2, 3, 4, 6, 7, 0, 1, 5}, // 0b00100011 (35)|Left-PC: 5
		{0, 1, 3, 4, 6, 7, 2, 5}, // 0b00100100 (36)|Left-PC: 6
		{1, 3, 4, 6, 7, 0, 2, 5}, // 0b00100101 (37)|Left-PC: 5
		{0, 3, 4, 6, 7, 1, 2, 5}, // 0b00100110 (38)|Left-PC: 5
		{3, 4, 6, 7, 0, 1, 2, 5}, // 0b00100111 (39)|Left-PC: 4
		{0, 1, 2, 4, 6, 7, 3, 5}, // 0b00101000 (40)|Left-PC: 6
		{1, 2, 4, 6, 7, 0, 3, 5}, // 0b00101001 (41)|Left-PC: 5
		{0, 2, 4, 6, 7, 1, 3, 5}, // 0b00101010 (42)|Left-PC: 5
		{2, 4, 6, 7, 0, 1, 3, 5}, // 0b00101011 (43)|Left-PC: 4
		{0, 1, 4, 6, 7, 2, 3, 5}, // 0b00101100 (44)|Left-PC: 5
		{1, 4, 6, 7, 0, 2, 3, 5}, // 0b00101101 (45)|Left-PC: 4
		{0, 4, 6, 7, 1, 2, 3, 5}, // 0b00101110 (46)|Left-PC: 4
		{4, 6, 7, 0, 1, 2, 3, 5}, // 0b00101111 (47)|Left-PC: 3
		{0, 1, 2, 3, 6, 7, 4, 5}, // 0b00110000 (48)|Left-PC: 6
		{1, 2, 3, 6, 7, 0, 4, 5}, // 0b00110001 (49)|Left-PC: 5
		{0, 2, 3, 6, 7, 1, 4, 5}, // 0b00110010 (50)|Left-PC: 5
		{2, 3, 6, 7, 0, 1, 4, 5}, // 0b00110011 (51)|Left-PC: 4
		{0, 1, 3, 6, 7, 2, 4, 5}, // 0b00110100 (52)|Left-PC: 5
		{1, 3, 6, 7, 0, 2, 4, 5}, // 0b00110101 (53)|Left-PC: 4
		{0, 3, 6, 7, 1, 2, 4, 5}, // 0b00110110 (54)|Left-PC: 4
		{3, 6, 7, 0, 1, 2, 4, 5}, // 0b00110111 (55)|Left-PC: 3
		{0, 1, 2, 6, 7, 3, 4, 5}, // 0b00111000 (56)|Left-PC: 5
		{1, 2, 6, 7, 0, 3, 4, 5}, // 0b00111001 (57)|Left-PC: 4
		{0, 2, 6, 7, 1, 3, 4, 5}, // 0b00111010 (58)|Left-PC: 4
		{2, 6, 7, 0, 1, 3, 4, 5}, // 0b00111011 (59)|Left-PC: 3
		{0, 1, 6, 7, 2, 3, 4, 5}, // 0b00111100 (60)|Left-PC: 4
		{1, 6, 7, 0, 2, 3, 4, 5}, // 0b00111101 (61)|Left-PC: 3
		{0, 6, 7, 1, 2, 3, 4, 5}, // 0b00111110 (62)|Left-PC: 3
		{6, 7, 0, 1, 2, 3, 4, 5}, // 0b00111111 (63)|Left-PC: 2
		{0, 1, 2, 3, 4, 5, 7, 6}, // 0b01000000 (64)|Left-PC: 7
		{1, 2, 3, 4, 5, 7, 0, 6}, // 0b01000001 (65)|Left-PC: 6
		{0, 2, 3, 4, 5, 7, 1, 6}, // 0b01000010 (66)|Left-PC: 6
		{2, 3, 4, 5, 7, 0, 1, 6}, // 0b01000011 (67)|Left-PC: 5
		{0, 1, 3, 4, 5, 7, 2, 6}, // 0b01000100 (68)|Left-PC: 6
		{1, 3, 4, 5, 7, 0, 2, 6}, // 0b01000101 (69)|Left-PC: 5
		{0, 3, 4, 5, 7, 1, 2, 6}, // 0b01000110 (70)|Left-PC: 5
		{3, 4, 5, 7, 0, 1, 2, 6}, // 0b01000111 (71)|Left-PC: 4
		{0, 1, 2, 4, 5, 7, 3, 6}, // 0b01001000 (72)|Left-PC: 6
		{1, 2, 4, 5, 7, 0, 3, 6}, // 0b01001001 (73)|Left-PC: 5
		{0, 2, 4, 5, 7, 1, 3, 6}, // 0b01001010 (74)|Left-PC: 5
		{2, 4, 5, 7, 0, 1, 3, 6}, // 0b01001011 (75)|Left-PC: 4
		{0, 1, 4, 5, 7, 2, 3, 6}, // 0b01001100 (76)|Left-PC: 5
		{1, 4, 5, 7, 0, 2, 3, 6}, // 0b01001101 (77)|Left-PC: 4
		{0, 4, 5, 7, 1, 2, 3, 6}, // 0b01001110 (78)|Left-PC: 4
		{4, 5, 7, 0, 1, 2, 3, 6}, // 0b01001111 (79)|Left-PC: 3
		{0, 1, 2, 3, 5, 7, 4, 6}, // 0b01010000 (80)|Left-PC: 6
		{1, 2, 3, 5, 7, 0, 4, 6}, // 0b01010001 (81)|Left-PC: 5
		{0, 2, 3, 5, 7, 1, 4, 6}, // 0b01010010 (82)|Left-PC: 5
		{2, 3, 5, 7, 0, 1, 4, 6}, // 0b01010011 (83)|Left-PC: 4
		{0, 1, 3, 5, 7, 2, 4, 6}, // 0b01010100 (84)|Left-PC: 5
		{1, 3, 5, 7, 0, 2, 4, 6}, // 0b01010101 (85)|Left-PC: 4
		{0, 3, 5, 7, 1, 2, 4, 6}, // 0b01010110 (86)|Left-PC: 4
		{3, 5, 7, 0, 1, 2, 4, 6}, // 0b01010111 (87)|Left-PC: 3
		{0, 1, 2, 5, 7, 3, 4, 6}, // 0b01011000 (88)|Left-PC: 5
		{1, 2, 5, 7, 0, 3, 4, 6}, // 0b01011001 (89)|Left-PC: 4
		{0, 2, 5, 7, 1, 3, 4, 6}, // 0b01011010 (90)|Left-PC: 4
		{2, 5, 7, 0, 1, 3, 4, 6}, // 0b01011011 (91)|Left-PC: 3
		{0, 1, 5, 7, 2, 3, 4, 6}, // 0b01011100 (92)|Left-PC: 4
		{1, 5, 7, 0, 2, 3, 4, 6}, // 0b01011101 (93)|Left-PC: 3
		{0, 5, 7, 1, 2, 3, 4, 6}, // 0b01011110 (94)|Left-PC: 3
		{5, 7, 0, 1, 2, 3, 4, 6}, // 0b01011111 (95)|Left-PC: 2
		{0, 1, 2, 3, 4, 7, 5, 6}, // 0b01100000 (96)|Left-PC: 6
		{1, 2, 3, 4, 7, 0, 5, 6}, // 0b01100001 (97)|Left-PC: 5
		{0, 2, 3, 4, 7, 1, 5, 6}, // 0b01100010 (98)|Left-PC: 5
		{2, 3, 4, 7, 0, 1, 5, 6}, // 0b01100011 (99)|Left-PC: 4
		{0, 1, 3, 4, 7, 2, 5, 6}, // 0b01100100 (100)|Left-PC: 5
		{1, 3, 4, 7, 0, 2, 5, 6}, // 0b01100101 (101)|Left-PC: 4
		{0, 3, 4, 7, 1, 2, 5, 6}, // 0b01100110 (102)|Left-PC: 4
		{3, 4, 7, 0, 1, 2, 5, 6}, // 0b01100111 (103)|Left-PC: 3
		{0, 1, 2, 4, 7, 3, 5, 6}, // 0b01101000 (104)|Left-PC: 5
		{1, 2, 4, 7, 0, 3, 5, 6}, // 0b01101001 (105)|Left-PC: 4
		{0, 2, 4, 7, 1, 3, 5, 6}, // 0b01101010 (106)|Left-PC: 4
		{2, 4, 7, 0, 1, 3, 5, 6}, // 0b01101011 (107)|Left-PC: 3
		{0, 1, 4, 7, 2, 3, 5, 6}, // 0b01101100 (108)|Left-PC: 4
		{1, 4, 7, 0, 2, 3, 5, 6}, // 0b01101101 (109)|Left-PC: 3
		{0, 4, 7, 1, 2, 3, 5, 6}, // 0b01101110 (110)|Left-PC: 3
		{4, 7, 0, 1, 2, 3, 5, 6}, // 0b01101111 (111)|Left-PC: 2
		{0, 1, 2, 3, 7, 4, 5, 6}, // 0b01110000 (112)|Left-PC: 5
		{1, 2, 3, 7, 0, 4, 5, 6}, // 0b01110001 (113)|Left-PC: 4
		{0, 2, 3, 7, 1, 4, 5, 6}, // 0b01110010 (114)|Left-PC: 4
		{2, 3, 7, 0, 1, 4, 5, 6}, // 0b01110011 (115)|Left-PC: 3
		{0, 1, 3, 7, 2, 4, 5, 6}, // 0b01110100 (116)|Left-PC: 4
		{1, 3, 7, 0, 2, 4, 5, 6}, // 0b01110101 (117)|Left-PC: 3
		{0, 3, 7, 1, 2, 4, 5, 6}, // 0b01110110 (118)|Left-PC: 3
		{3, 7, 0, 1, 2, 4, 5, 6}, // 0b01110111 (119)|Left-PC: 2
		{0, 1, 2, 7, 3, 4, 5, 6}, // 0b01111000 (120)|Left-PC: 4
		{1, 2, 7, 0, 3, 4, 5, 6}, // 0b01111001 (121)|Left-PC: 3
		{0, 2, 7, 1, 3, 4, 5, 6}, // 0b01111010 (122)|Left-PC: 3
		{2, 7, 0, 1, 3, 4, 5, 6}, // 0b01111011 (123)|Left-PC: 2
		{0, 1, 7, 2, 3, 4, 5, 6}, // 0b01111100 (124)|Left-PC: 3
		{1, 7, 0, 2, 3, 4, 5, 6}, // 0b01111101 (125)|Left-PC: 2
		{0, 7, 1, 2, 3, 4, 5, 6}, // 0b01111110 (126)|Left-PC: 2
		{7, 0, 1, 2, 3, 4, 5, 6}, // 0b01111111 (127)|Left-PC: 1
		{0, 1, 2, 3, 4, 5, 6, 7}, // 0b10000000 (128)|Left-PC: 7
		{1, 2, 3, 4, 5, 6, 0, 7}, // 0b10000001 (129)|Left-PC: 6
		{0, 2, 3, 4, 5, 6, 1, 7}, // 0b10000010 (130)|Left-PC: 6
		{2, 3, 4, 5, 6, 0, 1, 7}, // 0b10000011 (131)|Left-PC: 5
		{0, 1, 3, 4, 5, 6, 2, 7}, // 0b10000100 (132)|Left-PC: 6
		{1, 3, 4, 5, 6, 0, 2, 7}, // 0b10000101 (133)|Left-PC: 5
		{0, 3, 4, 5, 6, 1, 2, 7}, // 0b10000110 (134)|Left-PC: 5
		{3, 4, 5, 6, 0, 1, 2, 7}, // 0b10000111 (135)|Left-PC: 4
		{0, 1, 2, 4, 5, 6, 3, 7}, // 0b10001000 (136)|Left-PC: 6
		{1, 2, 4, 5, 6, 0, 3, 7}, // 0b10001001 (137)|Left-PC: 5
		{0, 2, 4, 5, 6, 1, 3, 7}, // 0b10001010 (138)|Left-PC: 5
		{2, 4, 5, 6, 0, 1, 3, 7}, // 0b10001011 (139)|Left-PC: 4
		{0, 1, 4, 5, 6, 2, 3, 7}, // 0b10001100 (140)|Left-PC: 5
		{1, 4, 5, 6, 0, 2, 3, 7}, // 0b10001101 (141)|Left-PC: 4
		{0, 4, 5, 6, 1, 2, 3, 7}, // 0b10001110 (142)|Left-PC: 4
		{4, 5, 6, 0, 1, 2, 3, 7}, // 0b10001111 (143)|Left-PC: 3
		{0, 1, 2, 3, 5, 6, 4, 7}, // 0b10010000 (144)|Left-PC: 6
		{1, 2, 3, 5, 6, 0, 4, 7}, // 0b10010001 (145)|Left-PC: 5
		{0, 2, 3, 5, 6, 1, 4, 7}, // 0b10010010 (146)|Left-PC: 5
		{2, 3, 5, 6, 0, 1, 4, 7}, // 0b10010011 (147)|Left-PC: 4
		{0, 1, 3, 5, 6, 2, 4, 7}, // 0b10010100 (148)|Left-PC: 5
		{1, 3, 5, 6, 0, 2, 4, 7}, // 0b10010101 (149)|Left-PC: 4
		{0, 3, 5, 6, 1, 2, 4, 7}, // 0b10010110 (150)|Left-PC: 4
		{3, 5, 6, 0, 1, 2, 4, 7}, // 0b10010111 (151)|Left-PC: 3
		{0, 1, 2, 5, 6, 3, 4, 7}, // 0b10011000 (152)|Left-PC: 5
		{1, 2, 5, 6, 0, 3, 4, 7}, // 0b10011001 (153)|Left-PC: 4
		{0, 2, 5, 6, 1, 3, 4, 7}, // 0b10011010 (154)|Left-PC: 4
		{2, 5, 6, 0, 1, 3, 4, 7}, // 0b10011011 (155)|Left-PC: 3
		{0, 1, 5, 6, 2, 3, 4, 7}, // 0b10011100 (156)|Left-PC: 4
		{1, 5, 6, 0, 2, 3, 4, 7}, // 0b10011101 (157)|Left-PC: 3
		{0, 5, 6, 1, 2, 3, 4, 7}, // 0b10011110 (158)|Left-PC: 3
		{5, 6, 0, 1, 2, 3, 4, 7}, // 0b10011111 (159)|Left-PC: 2
		{0, 1, 2, 3, 4, 6, 5, 7}, // 0b10100000 (160)|Left-PC: 6
		{1, 2, 3, 4, 6, 0, 5, 7}, // 0b10100001 (161)|Left-PC: 5
		{0, 2, 3, 4, 6, 1, 5, 7}, // 0b10100010 (162)|Left-PC: 5
		{2, 3, 4, 6, 0, 1, 5, 7}, // 0b10100011 (163)|Left-PC: 4
		{0, 1, 3, 4, 6, 2, 5, 7}, // 0b10100100 (164)|Left-PC: 5
		{1, 3, 4, 6, 0, 2, 5, 7}, // 0b10100101 (165)|Left-PC: 4
		{0, 3, 4, 6, 1, 2, 5, 7}, // 0b10100110 (166)|Left-PC: 4
		{3, 4, 6, 0, 1, 2, 5, 7}, // 0b10100111 (167)|Left-PC: 3
		{0, 1, 2, 4, 6, 3, 5, 7}, // 0b10101000 (168)|Left-PC: 5
		{1, 2, 4, 6, 0, 3, 5, 7}, // 0b10101001 (169)|Left-PC: 4
		{0, 2, 4, 6, 1, 3, 5, 7}, // 0b10101010 (170)|Left-PC: 4
		{2, 4, 6, 0, 1, 3, 5, 7}, // 0b10101011 (171)|Left-PC: 3
		{0, 1, 4, 6, 2, 3, 5, 7}, // 0b10101100 (172)|Left-PC: 4
		{1, 4, 6, 0, 2, 3, 5, 7}, // 0b10101101 (173)|Left-PC: 3
		{0, 4, 6, 1, 2, 3, 5, 7}, // 0b10101110 (174)|Left-PC: 3
		{4, 6, 0, 1, 2, 3, 5, 7}, // 0b10101111 (175)|Left-PC: 2
		{0, 1, 2, 3, 6, 4, 5, 7}, // 0b10110000 (176)|Left-PC: 5
		{1, 2, 3, 6, 0, 4, 5, 7}, // 0b10110001 (177)|Left-PC: 4
		{0, 2, 3, 6, 1, 4, 5, 7}, // 0b10110010 (178)|Left-PC: 4
		{2, 3, 6, 0, 1, 4, 5, 7}, // 0b10110011 (179)|Left-PC: 3
		{0, 1, 3, 6, 2, 4, 5, 7}, // 0b10110100 (180)|Left-PC: 4
		{1, 3, 6, 0, 2, 4, 5, 7}, // 0b10110101 (181)|Left-PC: 3
		{0, 3, 6, 1, 2, 4, 5, 7}, // 0b10110110 (182)|Left-PC: 3
		{3, 6, 0, 1, 2, 4, 5, 7}, // 0b10110111 (183)|Left-PC: 2
		{0, 1, 2, 6, 3, 4, 5, 7}, // 0b10111000 (184)|Left-PC: 4
		{1, 2, 6, 0, 3, 4, 5, 7}, // 0b10111001 (185)|Left-PC: 3
		{0, 2, 6, 1, 3, 4, 5, 7}, // 0b10111010 (186)|Left-PC: 3
		{2, 6, 0, 1, 3, 4, 5, 7}, // 0b10111011 (187)|Left-PC: 2
		{0, 1, 6, 2, 3, 4, 5, 7}, // 0b10111100 (188)|Left-PC: 3
		{1, 6, 0, 2, 3, 4, 5, 7}, // 0b10111101 (189)|Left-PC: 2
		{0, 6, 1, 2, 3, 4, 5, 7}, // 0b10111110 (190)|Left-PC: 2
		{6, 0, 1, 2, 3, 4, 5, 7}, // 0b10111111 (191)|Left-PC: 1
		{0, 1, 2, 3, 4, 5, 6, 7}, // 0b11000000 (192)|Left-PC: 6
		{1, 2, 3, 4, 5, 0, 6, 7}, // 0b11000001 (193)|Left-PC: 5
		{0, 2, 3, 4, 5, 1, 6, 7}, // 0b11000010 (194)|Left-PC: 5
		{2, 3, 4, 5, 0, 1, 6, 7}, // 0b11000011 (195)|Left-PC: 4
		{0, 1, 3, 4, 5, 2, 6, 7}, // 0b11000100 (196)|Left-PC: 5
		{1, 3, 4, 5, 0, 2, 6, 7}, // 0b11000101 (197)|Left-PC: 4
		{0, 3, 4, 5, 1, 2, 6, 7}, // 0b11000110 (198)|Left-PC: 4
		{3, 4, 5, 0, 1, 2, 6, 7}, // 0b11000111 (199)|Left-PC: 3
		{0, 1, 2, 4, 5, 3, 6, 7}, // 0b11001000 (200)|Left-PC: 5
		{1, 2, 4, 5, 0, 3, 6, 7}, // 0b11001001 (201)|Left-PC: 4
		{0, 2, 4, 5, 1, 3, 6, 7}, // 0b11001010 (202)|Left-PC: 4
		{2, 4, 5, 0, 1, 3, 6, 7}, // 0b11001011 (203)|Left-PC: 3
		{0, 1, 4, 5, 2, 3, 6, 7}, // 0b11001100 (204)|Left-PC: 4
		{1, 4, 5, 0, 2, 3, 6, 7}, // 0b11001101 (205)|Left-PC: 3
		{0, 4, 5, 1, 2, 3, 6, 7}, // 0b11001110 (206)|Left-PC: 3
		{4, 5, 0, 1, 2, 3, 6, 7}, // 0b11001111 (207)|Left-PC: 2
		{0, 1, 2, 3, 5, 4, 6, 7}, // 0b11010000 (208)|Left-PC: 5
		{1, 2, 3, 5, 0, 4, 6, 7}, // 0b11010001 (209)|Left-PC: 4
		{0, 2, 3, 5, 1, 4, 6, 7}, // 0b11010010 (210)|Left-PC: 4
		{2, 3, 5, 0, 1, 4, 6, 7}, // 0b11010011 (211)|Left-PC: 3
		{0, 1, 3, 5, 2, 4, 6, 7}, // 0b11010100 (212)|Left-PC: 4
		{1, 3, 5, 0, 2, 4, 6, 7}, // 0b11010101 (213)|Left-PC: 3
		{0, 3, 5, 1, 2, 4, 6, 7}, // 0b11010110 (214)|Left-PC: 3
		{3, 5, 0, 1, 2, 4, 6, 7}, // 0b11010111 (215)|Left-PC: 2
		{0, 1, 2, 5, 3, 4, 6, 7}, // 0b11011000 (216)|Left-PC: 4
		{1, 2, 5, 0, 3, 4, 6, 7}, // 0b11011001 (217)|Left-PC: 3
		{0, 2, 5, 1, 3, 4, 6, 7}, // 0b11011010 (218)|Left-PC: 3
		{2, 5, 0, 1, 3, 4, 6, 7}, // 0b11011011 (219)|Left-PC: 2
		{0, 1, 5, 2, 3, 4, 6, 7}, // 0b11011100 (220)|Left-PC: 3
		{1, 5, 0, 2, 3, 4, 6, 7}, // 0b11011101 (221)|Left-PC: 2
		{0, 5, 1, 2, 3, 4, 6, 7}, // 0b11011110 (222)|Left-PC: 2
		{5, 0, 1, 2, 3, 4, 6, 7}, // 0b11011111 (223)|Left-PC: 1
		{0, 1, 2, 3, 4, 5, 6, 7}, // 0b11100000 (224)|Left-PC: 5
		{1, 2, 3, 4, 0, 5, 6, 7}, // 0b11100001 (225)|Left-PC: 4
		{0, 2, 3, 4, 1, 5, 6, 7}, // 0b11100010 (226)|Left-PC: 4
		{2, 3, 4, 0, 1, 5, 6, 7}, // 0b11100011 (227)|Left-PC: 3
		{0, 1, 3, 4, 2, 5, 6, 7}, // 0b11100100 (228)|Left-PC: 4
		{1, 3, 4, 0, 2, 5, 6, 7}, // 0b11100101 (229)|Left-PC: 3
		{0, 3, 4, 1, 2, 5, 6, 7}, // 0b11100110 (230)|Left-PC: 3
		{3, 4, 0, 1, 2, 5, 6, 7}, // 0b11100111 (231)|Left-PC: 2
		{0, 1, 2, 4, 3, 5, 6, 7}, // 0b11101000 (232)|Left-PC: 4
		{1, 2, 4, 0, 3, 5, 6, 7}, // 0b11101001 (233)|Left-PC: 3
		{0, 2, 4, 1, 3, 5, 6, 7}, // 0b11101010 (234)|Left-PC: 3
		{2, 4, 0, 1, 3, 5, 6, 7}, // 0b11101011 (235)|Left-PC: 2
		{0, 1, 4, 2, 3, 5, 6, 7}, // 0b11101100 (236)|Left-PC: 3
		{1, 4, 0, 2, 3, 5, 6, 7}, // 0b11101101 (237)|Left-PC: 2
		{0, 4, 1, 2, 3, 5, 6, 7}, // 0b11101110 (238)|Left-PC: 2
		{4, 0, 1, 2, 3, 5, 6, 7}, // 0b11101111 (239)|Left-PC: 1
		{0, 1, 2, 3, 4, 5, 6, 7}, // 0b11110000 (240)|Left-PC: 4
		{1, 2, 3, 0, 4, 5, 6, 7}, // 0b11110001 (241)|Left-PC: 3
		{0, 2, 3, 1, 4, 5, 6, 7}, // 0b11110010 (242)|Left-PC: 3
		{2, 3, 0, 1, 4, 5, 6, 7}, // 0b11110011 (243)|Left-PC: 2
		{0, 1, 3, 2, 4, 5, 6, 7}, // 0b11110100 (244)|Left-PC: 3
		{1, 3, 0, 2, 4, 5, 6, 7}, // 0b11110101 (245)|Left-PC: 2
		{0, 3, 1, 2, 4, 5, 6, 7}, // 0b11110110 (246)|Left-PC: 2
		{3, 0, 1, 2, 4, 5, 6, 7}, // 0b11110111 (247)|Left-PC: 1
		{0, 1, 2, 3, 4, 5, 6, 7}, // 0b11111000 (248)|Left-PC: 3
		{1, 2, 0, 3, 4, 5, 6, 7}, // 0b11111001 (249)|Left-PC: 2
		{0, 2, 1, 3, 4, 5, 6, 7}, // 0b11111010 (250)|Left-PC: 2
		{2, 0, 1, 3, 4, 5, 6, 7}, // 0b11111011 (251)|Left-PC: 1
		{0, 1, 2, 3, 4, 5, 6, 7}, // 0b11111100 (252)|Left-PC: 2
		{1, 0, 2, 3, 4, 5, 6, 7}, // 0b11111101 (253)|Left-PC: 1
		{0, 1, 2, 3, 4, 5, 6, 7}, // 0b11111110 (254)|Left-PC: 1
		{0, 1, 2, 3, 4, 5, 6, 7}  // 0b11111111 (255)|Left-PC: 0
	};
};

/*排序类，其中包含两个静态成员*/
class bubble {
public:
	static float sum;
	static float max;
	bubble();
	void bubblesum();
	void bubblemax();
	int bubblebasesum();
	int bubblebasemax();
	int bubblebasesort();
	int bubblethreadssum();
	int bubblethreadsmax();
	int bubblethreadssort();
	void sortinsert(float* p1, float* p2);
	bool sortmerge(int);
	int timesum = 0;
	int timemax = 0;
	int timesort = 0;
};
float bubble::sum = 0.0f;
float bubble::max = 0.0f;
/***************************************数组的初始化和打乱顺序*************************************************/
bubble::bubble()
{
	for (size_t i = 0; i < DATANUM; i++)
	{
		rawFloatData[i] = float(i + 1);
	}
	for (size_t i = 0; i < DATANUM; i++)
	{
		sortmid[i] = 0;
	}

	srand(1234567);//伪随机数种子，注意不能用时间做种子，否则两机无法同步
	float  tmp;
	size_t  T = DATANUM;
	while (T--)
	{
		size_t i, j;
		i = (size_t(rand()) * size_t(rand())) % DATANUM;
		j = (size_t(rand()) * size_t(rand())) % DATANUM;
		tmp = rawFloatData[i];
		rawFloatData[i] = rawFloatData[j];
		rawFloatData[j] = tmp;
	}
	cout << "初始化结束" << endl;
}
/***************************************串行求和*************************************************/
void bubble::bubblesum()
{
	for (size_t i = 0; i < MAX_THREADS; i++)
	{
		summid[i] = 0.0f;
	}
	for (size_t j = 0; j < MAX_THREADS; j++)
	{
		for (size_t i = 0; i < SUBDATANUM; i++)
		{
			summid[j] += sqrt(sqrt(rawFloatData[j * SUBDATANUM + i]));
		}
	}
	for (size_t i = 0; i < MAX_THREADS; i++)
	{
		sum += summid[i];
	}
}
/***************************************串行求最大值*************************************************/
void bubble::bubblemax()
{
	for (size_t i = 0; i < DATANUM; i++)
	{
		if (rawFloatData[i] > max)
		{
			max = rawFloatData[i];
		}
	}
}
/***************************************串行快速排序*************************************************/
void SWAP(float* x, float* y) {
	float temp = *x;
	*x = *y;
	*y = temp;
}

void quicksort(float arr[], int left, int right) {
	int i, j;
	float s;

	if (left < right) {
		s = arr[(left + right) / 2];
		i = left - 1;
		j = right + 1;

		while (1) {
			do {
				i++;
			} while (arr[i] < s);

			do {
				j--;
			} while (arr[j] > s);

			if (i >= j)
				break;

			SWAP(&arr[i], &arr[j]);
		}

		quicksort(arr, left, i - 1);
		quicksort(arr, j + 1, right);
	}
}
/***************************************并行快速排序*************************************************/

void avx256_quicksort(float arr[], int left, int right) {
	int i, j;
	float s;

	if (left < right) {
		s = arr[(left + right) / 2];
		i = left - 1;
		j = right + 1;

		__m256 s_vector = _mm256_set1_ps(s);

		while (1) {
			do {
				i++;
			} while (arr[i] < s);

			do {
				j--;
			} while (arr[j] > s);

			if (i >= j)
				break;

			// Use AVX256 to swap elements
			__m256 temp_a = _mm256_loadu_ps(&arr[i]);
			__m256 temp_b = _mm256_loadu_ps(&arr[j]);
			_mm256_storeu_ps(&arr[i], temp_b);
			_mm256_storeu_ps(&arr[j], temp_a);
		}

		avx256_quicksort(arr, left, i - 1);
		avx256_quicksort(arr, j + 1, right);
	}
}

void AvxPartition(float* origData, float*& writeLeft, float*& writeRight, __m256& pivotVector) {
	// load data into vector
	__m256 data = _mm256_loadu_ps((float*)(origData));
	// create mask
	//__m256 compared = _mm256_cmpgt_epi32(data, pivotVector);
	__m256 compared = _mm256_cmp_ps(data, pivotVector, _CMP_GT_OQ);
	int mask = _mm256_movemask_ps(*(__m256*) & compared);
	// permute data with permutation table
	data = _mm256_permutevar8x32_ps(data, _mm256_loadu_si256((__m256i*)PermLookupTable32Bit::permTable[mask]));
	// store into respective places
	_mm256_storeu_ps((float*)writeLeft, data);
	_mm256_storeu_ps((float*)writeRight, data);
	// count the amount of values > pivot
	int nLargerPivot = _mm_popcnt_u32(mask);
	writeRight -= nLargerPivot;
	writeLeft += 8 - nLargerPivot;

}

constexpr int floatIn256 = (sizeof(__m256i) / sizeof(float));

// assumes that pivot element is at a[right]

float* VecotrizedPartition(float* left, float* right) {
	float piv = *right;
	__m256 P = _mm256_set1_ps(piv);

	// Prepare the tmp array
	float tmpArray[24];
	float* const tmpStart = &tmpArray[0];			// start address of the tmp array
	float* const tmpEnd = &tmpArray[24];			// last + 1 addres of the tmp array
	float* tmpLeft = tmpStart;					// where to start writing on the left side (<= pivot)
	float* tmpRight = tmpEnd - floatIn256;		// where to start writing on the right side (> pivot)
	// Read most left and most right block into the tmp array, so we can implace correctly
	AvxPartition(left, tmpLeft, tmpRight, P);
	AvxPartition(right - floatIn256, tmpLeft, tmpRight, P);
	tmpRight += floatIn256;	// reset the offset from the beginning

	float* writeLeft = left;						// write values <= pivot
	float* writeRight = right - floatIn256;		// write values  > pivot
	float* readLeft = left + floatIn256;		// pop N ints from <= pivot side
	float* readRight = right - 2 * floatIn256;	// pop N ints from  > pivot side

	// as long as there are at least N unread, unsorted entries
	while (readRight >= readLeft) {
		float* nextPtr;
		// decide wether to read the left or the right next block of data
		if ((readLeft - writeLeft) <= (writeRight - readRight)) {
			nextPtr = readLeft;
			readLeft += floatIn256;
		}
		else {
			nextPtr = readRight;
			readRight -= floatIn256;
		}
		AvxPartition(nextPtr, writeLeft, writeRight, P);
	}

	// for each element not yet sorted by avx, put into tmp array
	for (readLeft; readLeft < readRight + floatIn256; readLeft++) {
		*readLeft <= piv ? *tmpLeft++ = *readLeft : *--tmpRight = *readLeft;
	}

	// copy the contents of the tmp array to the actual data
	// left side
	size_t leftTmpSize = tmpLeft - tmpStart;
	memcpy(writeLeft, tmpStart, leftTmpSize * sizeof(float));
	writeLeft += leftTmpSize; // advance pointer
	// right side
	size_t rightTmpSize = tmpEnd - tmpRight;
	memcpy(writeLeft, tmpRight, rightTmpSize * sizeof(float));

	// lastly, swap the pivot with the first element in the data that is > pivot
	*right = *writeLeft;
	*writeLeft = piv;

	return writeLeft;
}

void insertionSort(float* data, int64_t start, int64_t stop) {
	for (size_t i = start + 1; i <= stop; i++) {
		float key = data[i];
		size_t j = i;
		while (j > start && data[j - 1] > key) {
			data[j] = data[j - 1];
			j = j - 1;
		}
		data[j] = key;
	}
}

void quickSortAVX(float* data, int64_t beg, int64_t end) {
	// refer to https://github.com/fhnw-pac/QuickSort-AVX
	int64_t len = end - beg + 1;
	// Only use quicksort when problem set is larger than 16
	if (len > 16) {
		int64_t bound = (size_t)(VecotrizedPartition(&data[beg], &data[end]) - &data[0]);
		quickSortAVX(data, beg, bound - 1); // sort left side
		quickSortAVX(data, bound + 1, end); // sort right side
	}
	else if (len > 1) {
		insertionSort(data, beg, end);
	}
}

/***************************************串行求和*************************************************/

int bubble::bubblebasesum()
{
	LARGE_INTEGER start, end;
	QueryPerformanceCounter(&start);//start  
	bubblesum();
	cout << "单机串行求和结果为:" << sum << endl;
	QueryPerformanceCounter(&end);//end
	std::cout << "单机串行求和时间为:" << (end.QuadPart - start.QuadPart) << endl;
	timesum = (end.QuadPart - start.QuadPart);
	sum = 0.0;
	max = 0.0;
	return timesum;
}
int bubble::bubblebasemax()
{
	LARGE_INTEGER start, end;
	QueryPerformanceCounter(&start);//start  
	bubblemax();
	cout << "单机串行求最大值结果为：" << max << endl;
	QueryPerformanceCounter(&end);//end
	std::cout << "单机串行求最大值时间为：" << (end.QuadPart - start.QuadPart) << endl;
	timemax = (end.QuadPart - start.QuadPart);
	sum = 0.0;
	max = 0.0;
	return timemax;
}
int bubble::bubblebasesort()
{
	LARGE_INTEGER start, end;
	QueryPerformanceCounter(&start);//start  
	quicksort(rawFloatData, 0, DATANUM - 1);
	bool comp = true;
	for (size_t i = 0; i < DATANUM - 1; i++)//判断排序是否正确
	{

		if (rawFloatData[i] > rawFloatData[i + 1])
		{
			comp = false;
			break;
		}
	}
	if (comp)
	{
		cout << "单机串行排序正确" << endl;
	}
	else
	{
		cout << "单机串行排序错误" << endl;
	}

	QueryPerformanceCounter(&end);//end
	std::cout << "单机串行排序时间为：" << (end.QuadPart - start.QuadPart) << endl;
	timesort = (end.QuadPart - start.QuadPart);
	sum = 0.0;
	max = 0.0;
	return timesort;
}
/***************************************多线程（64）排序*************************************************/
// 使用AVX指令集的快速排序算法
DWORD WINAPI Threadsort(LPVOID lpParameter)
{
	float* p = (float*)lpParameter;
	//avx256_quicksort(p, 0, SUBDATANUM - 1);
	quickSortAVX(p, 0, SUBDATANUM - 1);
	return 0;
}
/***************************************多线程（64）求和*************************************************/

DWORD WINAPI Threadsum(LPVOID lpParameter) {
	WaitForSingleObject(g_hHandle, INFINITE);

	float* p = (float*)lpParameter;
	long long int j = (p - &rawFloatData[0]) / SUBDATANUM;

	const size_t simdSize = 4;  // 假设是单精度（float）数据和 SSE
	__m128 sumVector = _mm_setzero_ps();

	for (size_t i = 0; i < SUBDATANUM; i += simdSize) {
		__m128 dataVector = _mm_loadu_ps(&p[i]);

		// 使用 SSE 指令进行平方根计算
		dataVector = _mm_sqrt_ps(_mm_sqrt_ps(dataVector));

		// 使用 SSE 指令进行累加
		sumVector = _mm_add_ps(sumVector, dataVector);
	}

	// 使用 SSE 指令进行水平求和
	__m128 shuf = _mm_shuffle_ps(sumVector, sumVector, _MM_SHUFFLE(2, 3, 0, 1));
	sumVector = _mm_add_ps(sumVector, shuf);
	shuf = _mm_movehl_ps(shuf, sumVector);
	sumVector = _mm_add_ss(sumVector, shuf);

	// 将结果存储在 summid[j] 中
	_mm_store_ss(&summid[j], sumVector);

	ReleaseMutex(g_hHandle);
	return 0;
}
/***************************************多线程（64）求最大值*************************************************/
DWORD WINAPI Threadmax(LPVOID lpParameter)
{
	float* p = (float*)lpParameter;
	int j = (p - rawFloatData) / SUBDATANUM;
	WaitForSingleObject(g_hHandle, INFINITE);
	for (size_t i = 0; i < SUBDATANUM; i++)
	{
		if (maxmid[j] < p[i])
		{
			maxmid[j] = p[i];
		}
	}
	ReleaseMutex(g_hHandle);
	return 0;
}
/*插入排序函数，是重排序的基本单元，对两个有序数组重排序*/
void bubble::sortinsert(float* p1, float* p2)
{
	long long int length = p2 - p1;
	long long int i = 0;
	long long int j = 0;
	while (i < length && j < length)
	{
		if (p1[i] <= p2[j])
		{
			sortmid[i + j] = p1[i];
			i++;
		}
		else
		{
			sortmid[i + j] = p2[j];
			j++;
		}
	}
	while (i < length)
	{
		sortmid[i + j] = p1[i];
		i++;
	}
	while (j < length)
	{
		sortmid[i + j] = p2[j];
		j++;
	}
	for (size_t i = 0; i < 2 * length; i++)
	{
		p1[i] = sortmid[i];
	}
}
/***************************************重排序（当线程较多时，会递归执行），同时验证排序的正确性*************************************************/
bool bubble::sortmerge(int Threads)
{
	bool comp = true;
	if (Threads > 1)
	{
		sortmerge(Threads / 2);
	}
	for (size_t i = 0; i < MAX_THREADS / Threads; i += 2)
	{
		sortinsert(&rawFloatData[i * Threads * SUBDATANUM], &rawFloatData[(i + 1) * Threads * SUBDATANUM]);
	}
	if (Threads == MAX_THREADS / 2)
	{
		for (size_t i = 0; i < DATANUM - 1; i++)
		{
			if (rawFloatData[i] > rawFloatData[i + 1])
			{
				std::cout << i;
				printf(": %.1f  %.1f\n", rawFloatData[i], rawFloatData[i + 1]);
				comp = false;
				break;
			}
		}
	}
	return comp;
}
int bubble::bubblethreadssum()
{
	for (size_t i = 0; i < MAX_THREADS; i++)
	{
		summid[i] = 0.0f;
	}
	LARGE_INTEGER start, end;
	QueryPerformanceCounter(&start);//start  
	g_hHandle = CreateMutex(NULL, FALSE, NULL);
	for (size_t i = 0; i < MAX_THREADS; i++)
	{
		//hSemaphores[i] = CreateSemaphore(NULL, 0, 1, NULL);//CreateEvent(NULL,TRUE,FALSE)等价
		hThreads[i] = CreateThread(
			NULL,
			0,
			Threadsum,
			&rawFloatData[i * SUBDATANUM],
			0,
			NULL);
	}
	WaitForMultipleObjects(MAX_THREADS, hThreads, TRUE, INFINITE);
	for (size_t i = 0; i < MAX_THREADS; i++)
	{
		sum += summid[i];
	}
	cout << "单机并行求和结果为:" << sum << endl;
	QueryPerformanceCounter(&end);//end
	std::cout << "单机并行求和时间为：" << (end.QuadPart - start.QuadPart) << endl;
	timesum = (end.QuadPart - start.QuadPart);
	for (size_t i = 0; i < MAX_THREADS; i++)
	{
		CloseHandle(hThreads[i]);
	}
	sum = 0.0f;
	max = 0.0f;
	return timesum;
}
int bubble::bubblethreadsmax()
{
	LARGE_INTEGER start, end;
	QueryPerformanceCounter(&start);//start  
	g_hHandle = CreateMutex(NULL, FALSE, NULL);
	for (size_t i = 0; i < MAX_THREADS; i++)
	{
		hThreads[i] = CreateThread(
			NULL,
			0,
			Threadmax,
			&rawFloatData[i * SUBDATANUM],
			0,
			NULL);
	}
	WaitForMultipleObjects(MAX_THREADS, hThreads, true, INFINITE);
	for (size_t i = 0; i < MAX_THREADS; i++)
	{
		if (max < maxmid[i])
		{
			max = maxmid[i];
		}
	}
	cout << "单机并行求最大值结果为：" << max << endl;
	QueryPerformanceCounter(&end);//end
	std::cout << "单机并行求最大值时间为：" << (end.QuadPart - start.QuadPart) << endl;
	timemax = (end.QuadPart - start.QuadPart);
	for (size_t i = 0; i < MAX_THREADS; i++)
	{
		CloseHandle(hThreads[i]);
	}
	return timemax;
}
int bubble::bubblethreadssort()
{
	for (size_t i = 0; i < MAX_THREADS; i++)
	{
		summid[i] = 0.0f;
	}
	LARGE_INTEGER start, end;
	QueryPerformanceCounter(&start);//start  
	g_hHandle = CreateMutex(NULL, FALSE, NULL);
	for (int i = 0; i < MAX_THREADS; i++)
	{

		hThreads[i] = CreateThread(
			NULL,// 安全属性标记
			0,// 默认栈空间
			Threadsort,//回调函数
			&rawFloatData[i * SUBDATANUM],// 回调函数句柄
			0,
			NULL);
	}
	WaitForMultipleObjects(MAX_THREADS, hThreads, true, INFINITE);	//当所有线程返回时才能结束
	if (sortmerge(MAX_THREADS / 2))
	{
		cout << "单机并行排序正确" << endl;
	}
	else
	{
		cout << "单机并行排序错误" << endl;
	}
	QueryPerformanceCounter(&end);//end
	std::cout << "单机并行排序时间为：" << (end.QuadPart - start.QuadPart) << endl;
	timesort = (end.QuadPart - start.QuadPart);
	for (size_t i = 0; i < MAX_THREADS; i++)
	{
		CloseHandle(hThreads[i]);
	}
	sum = 0.0f;
	max = 0.0f;
	return timesort;
}
int main()
{
	int timemax1, timemax2, timesum1, timesum2, timesort1, timesort2;
	bubble test1;
	timesum1 = test1.bubblebasesum();//测试单机串行的运行情况
	timemax1 = test1.bubblebasemax();//测试单机串行的运行情况
	timesort1 = test1.bubblebasesort();//测试单机串行的运行情况
	cout << "单机串行总时间为：" << timesum1 + timemax1 + timesort1 << endl;
	cout << endl;
	bubble test2;
	timesum2 = test2.bubblethreadssum();//测试单机串行的运行情况
	timemax2 = test2.bubblethreadsmax();//测试单机并行的运行情况
	timesort2 = test2.bubblethreadssort();//测试单机并行的运行情况
	cout << "单机并行总时间为：" << timesum2 + timemax2 + timesort2 << endl;
	cout << endl;
	cout << "单机并行求和与单机串行求和加速比为：" << double(timesum1) / double(timesum2) << endl;
	cout << "单机并行求最大值与单机串行求最大值加速比为：" << double(timemax1) / double(timemax2) << endl;
	cout << "单机并行排序与单机串行排序加速比为：" << double(timesort1) / double(timesort2) << endl;
	cout << "单机并行与单机串行总加速比为：" << double(timesum1 + timemax1 + timesort1) / double(timesum2 + timemax2 + timesort2) << endl;
	return 0;
}
