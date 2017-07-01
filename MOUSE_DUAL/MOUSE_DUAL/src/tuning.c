/*
 * CFile1.c
 *
 * Created: 6/21/2017 4:46:56 PM
 *  Author: Jeff Snyder
 */ 

#include "tuning.h"

uint8_t tuning = 0; //0-99

uint32_t scaledSemitoneDACvalue = 54613;
uint32_t scaledOctaveDACvalue = 655350;


//declaring this as const means it will go in the internal flash memory instead of the internal SRAM (where normal arrays go). Means it will fit, but also that we can't alter it, and it's slightly slower to access.
const uint32_t factoryTunings[100][50] = 
								{
								{12, 0, 10000, 20000, 30000, 40000, 50000, 60000, 70000, 80000, 90000, 100000, 110000}, //12tet
								{12, 0, 11100, 20300, 31600, 38600, 49800, 55100, 70200, 81300, 88400, 96800, 108800}, //overtone just
								{12, 0, 18500, 23000, 32500, 40500, 49800, 55100, 70200, 88500, 93000, 102500, 110500}, //kora1
								{12, 0, 11700, 19300, 31000, 38600, 50300, 57900, 69700, 77300, 89000, 96600, 108300}, //meantone
								{12, 0, 9000, 19200, 29400, 39000, 49800, 58800, 69600, 79200, 88800, 99600, 109200}, //werckmeister1
								{12, 0, 9600, 20400, 30000, 39600, 50400, 60000, 70200, 79200, 90000, 100200, 109800}, //werckmeister3
								{12, 0, 9500, 19000, 27500, 36000, 50200, 56000, 70400, 78700, 87000, 95000, 103000}, //georgian
								{12, 0, 11400, 20400, 31800, 40800, 49800, 61600, 70200, 81600, 90600, 102000, 111000}, //pythagorean
								{12, 0, 9000, 18000, 24000, 36000, 49800, 55200, 70200, 76000, 88000, 98000, 106000}, //mbira 
								{12 , 0, 9900, 20200, 36200, 46300, 65500, 75400, 86100, 94900, 99100, 104700, 112900, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, //yugoslavian bagpipe
								{12 , 0, 11173, 18240, 31564, 38631, 49805, 59022, 70196, 81369, 88436, 101760, 108827, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
								{12 , 0, 17665, 20391, 23961, 47078, 44352, 67469, 70196, 73765, 96883, 94156, 117274, 79245, 83774, 90566, 99623, 104151, 110943, 90586, 99609, 101955, 108631, 110978, 117654, 112933, 0, 0, 0, 0, 0, 0},
								{12 , 0, 20391, 23117, 31564, 38631, 47078, 49805, 70196, 81369, 93313, 96883, 108827, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
								{12 , 0, 10496, 20391, 31564, 38631, 49805, 55132, 70196, 84053, 88436, 96883, 108827, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
								{14 , 0, 11944, 20391, 26687, 38631, 43508, 49805, 58251, 70196, 76492, 88436, 96883, 108827, 113704, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
								{12 , 0, 18240, 20391, 23117, 26687, 47078, 49805, 68045, 70196, 72922, 76492, 96883, 108827, 113704, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
								{12 , 0, 11173, 20391, 31564, 38631, 49805, 51955, 70196, 81369, 88436, 101760, 108827, 108827, 113704, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
								{12 , 0, 11944, 20391, 26687, 38631, 49805, 59022, 70196, 82140, 88436, 96883, 108827, 108827, 113704, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
								{12 , 0, 6296, 20391, 26687, 38631, 49805, 55132, 70196, 76492, 88436, 96883, 104936, 108827, 113704, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
								{12 , 0, 9218, 20391, 32335, 38631, 55132, 59022, 70196, 74086, 90586, 96883, 108827, 79245, 83774, 90566, 99623, 104151, 110943, 90586, 99609, 101955, 108631, 110978, 117654, 112933, 0, 0, 0, 0, 0, 0},
								{22 , 0, 6296, 11173, 18240, 23117, 26687, 31564, 38631, 43508, 47078, 51955, 59022, 63899, 70196, 76492, 81369, 88436, 93313, 96883, 101760, 108827, 113704, 0, 0, 0, 0, 0, 0, 0, 0, 0},
								{25 , 0, 7067, 9218, 11173, 18240, 20391, 27458, 31564, 38631, 40782, 42737, 49805, 51955, 59022, 63128, 70196, 77263, 81369, 88436, 90586, 97654, 99609, 101760, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{12 , 0, 7067, 18240, 27458, 38631, 50576, 56872, 73372, 77263, 88436, 92326, 108827, 51955, 59022, 63128, 70196, 77263, 81369, 88436, 90586, 97654, 99609, 101760, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{18 , 0, 7067, 9218, 18240, 20391, 27458, 38631, 40782, 49805, 56872, 59022, 70196, 77263, 88436, 90586, 97654, 99609, 108827, 90586, 99609, 101955, 108631, 110978, 117654, 112933, 0, 0, 0, 0, 0, 0},
								{12 , 0, 7067, 20391, 31564, 38631, 49805, 59022, 70196, 81369, 88436, 96883, 108827, 51955, 59022, 63128, 70196, 77263, 81369, 88436, 90586, 97654, 99609, 101760, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{12 , 0, 11173, 20391, 27458, 38631, 49805, 59022, 70196, 81369, 88436, 97654, 108827, 51955, 59022, 63128, 70196, 77263, 81369, 88436, 90586, 97654, 99609, 101760, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{21 , 0, 11173, 18240, 20391, 23117, 26687, 31564, 38631, 43508, 49805, 58251, 61749, 70196, 76492, 81369, 88436, 93313, 96883, 99609, 101760, 108827, 99609, 101760, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{17 , 0, 6296, 12830, 20884, 26687, 34055, 41751, 49805, 56101, 62634, 70196, 76492, 83025, 91079, 96883, 104251, 111946, 96883, 99609, 101760, 108827, 99609, 101760, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{12 , 0, 11944, 20391, 26687, 43508, 49805, 58251, 70196, 76492, 88436, 101760, 113704, 83025, 91079, 96883, 104251, 111946, 96883, 99609, 101760, 108827, 99609, 101760, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{24 , 0, 5327, 11173, 20391, 23117, 26687, 31564, 34291, 35947, 38631, 47078, 49805, 55132, 59022, 64868, 70196, 81369, 84053, 88436, 90586, 96883, 99609, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{8 , 0, 20400, 41100, 71000, 100000, 120600, 140900, 191800, 35947, 38631, 47078, 49805, 55132, 59022, 64868, 70196, 81369, 84053, 88436, 90586, 96883, 99609, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{7 , 0, 20600, 34500, 52800, 72000, 81400, 102400, 191800, 35947, 38631, 47078, 49805, 55132, 59022, 64868, 70196, 81369, 84053, 88436, 90586, 96883, 99609, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{8 , 0, 19700, 28800, 48500, 69200, 98600, 120400, 142300, 35947, 38631, 47078, 49805, 55132, 59022, 64868, 70196, 81369, 84053, 88436, 90586, 96883, 99609, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{7 , 0, 69400, 99700, 120000, 140400, 161100, 171100, 142300, 35947, 38631, 47078, 49805, 55132, 59022, 64868, 70196, 81369, 84053, 88436, 90586, 96883, 99609, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{10 , 0, 20900, 41600, 68600, 92600, 114400, 121300, 137700, 153000, 182600, 47078, 49805, 55132, 59022, 64868, 70196, 81369, 84053, 88436, 90586, 96883, 99609, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{10 , 0, 12800, 31700, 50200, 69900, 88800, 114100, 134500, 143100, 160400, 47078, 49805, 55132, 59022, 64868, 70196, 81369, 84053, 88436, 90586, 96883, 99609, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{12 , 0, 87982, 18467, 120000, 36809, 50709, 156809, 69287, 170709, 87982, 106699, 189287, 55132, 59022, 64868, 70196, 81369, 84053, 88436, 90586, 96883, 99609, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{12 , 0, 37295, 18905, 51461, 37295, 51461, 87099, 70196, 106143, 87602, 120000, 106143, 55132, 59022, 64868, 70196, 81369, 84053, 88436, 90586, 96883, 99609, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{7 , 0, 20000, 38500, 50000, 70000, 90000, 108500, 70196, 106143, 87602, 120000, 106143, 55132, 59022, 64868, 70196, 81369, 84053, 88436, 90586, 96883, 99609, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{7 , 0, 23000, 32500, 50000, 70000, 93000, 102500, 70196, 106143, 87602, 120000, 106143, 55132, 59022, 64868, 70196, 81369, 84053, 88436, 90586, 96883, 99609, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{7 , 0, 18500, 40500, 50000, 70000, 88500, 110500, 40782, 49805, 56872, 59022, 70196, 77263, 88436, 90586, 97654, 99609, 108827, 90586, 99609, 101955, 108631, 110978, 117654, 112933, 0, 0, 0, 0, 0, 0},
								{7 , 0, 18500, 40500, 60500, 70000, 88500, 110500, 70196, 106143, 87602, 120000, 106143, 55132, 59022, 64868, 70196, 81369, 84053, 88436, 90586, 96883, 99609, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{7 , 0, 18500, 38900, 59300, 75600, 91400, 105100, 70196, 106143, 87602, 120000, 106143, 55132, 59022, 64868, 70196, 81369, 84053, 88436, 90586, 96883, 99609, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{21 , 0, 35500, 55400, 65000, 82900, 98200, 140000, 116900, 185000, 173200, 203800, 220700, 240000, 153100, 241500, 260000, 280400, 300800, 317100, 332900, 346600, 99609, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{7 , 0, 15607, 39283, 48164, 62401, 83801, 101032, 116900, 185000, 173200, 203800, 220700, 240000, 153100, 241500, 260000, 280400, 300800, 317100, 332900, 346600, 99609, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{21 , 0, 32300, 48000, 64400, 83000, 98100, 133000, 117900, 188800, 169700, 202500, 218900, 237100, 151700, 239000, 256900, 278700, 292300, 310500, 325600, 341700, 99609, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{7 , 0, 9800, 27100, 47200, 64200, 77100, 95200, 117900, 188800, 169700, 202500, 218900, 237100, 151700, 239000, 256900, 278700, 292300, 310500, 325600, 341700, 99609, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{21 , 0, 12600, 24300, 39900, 71300, 81800, 123200, 108200, 170600, 144300, 185800, 195500, 221900, 137100, 221000, 240000, 255600, 269900, 291800, 306900, 319700, 99609, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{21 , 0, 17400, 28900, 57500, 61200, 77000, 114600, 97600, 167800, 146700, 184800, 198700, 211500, 132600, 211700, 234800, 252800, 264600, 286000, 303200, 320500, 99609, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{12 , 0, 11944, 17042, 26985, 36975, 48403, 59515, 66959, 79556, 83025, 101266, 106503, 211500, 132600, 211700, 234800, 252800, 264600, 286000, 303200, 320500, 99609, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{12 , 0, 9022, 19609, 29805, 39414, 50000, 59022, 69804, 79022, 89609, 100000, 109218, 211500, 132600, 211700, 234800, 252800, 264600, 286000, 303200, 320500, 99609, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{12 , 0, 8880, 18240, 28136, 38631, 49805, 59700, 70196, 79076, 88436, 98331, 108827, 211500, 132600, 211700, 234800, 252800, 264600, 286000, 303200, 320500, 99609, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{12 , 0, 8436, 19218, 29414, 38436, 49805, 58240, 69609, 78632, 88827, 99609, 108631, 211500, 132600, 211700, 234800, 252800, 264600, 286000, 303200, 320500, 99609, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{12 , 0, 8534, 19902, 30586, 38729, 50098, 58338, 70196, 78143, 89609, 99414, 108631, 211500, 132600, 211700, 234800, 252800, 264600, 286000, 303200, 320500, 99609, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{12 , 0, 9895, 20391, 30286, 40782, 49805, 59700, 70196, 80091, 90586, 99609, 110978, 211500, 132600, 211700, 234800, 252800, 264600, 286000, 303200, 320500, 99609, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{12 , 0, 10300, 19800, 30100, 39600, 49500, 59400, 69300, 79200, 89100, 99000, 108900, 77263, 88436, 90586, 97654, 99609, 108827, 90586, 99609, 101955, 108631, 110978, 117654, 112933, 0, 0, 0, 0, 0, 0},		
								{12 , 0, 9022, 19609, 30195, 39218, 50195, 59022, 69804, 79609, 89414, 100196, 109022, 211500, 132600, 211700, 234800, 252800, 264600, 286000, 303200, 320500, 99609, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{12 , 0, 12248, 20587, 31564, 41271, 50489, 62151, 70000, 82248, 90782, 100976, 111955, 211500, 132600, 211700, 234800, 252800, 264600, 286000, 303200, 320500, 99609, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{12 , 0, 9658, 20684, 29658, 40000, 50342, 60000, 69658, 79316, 90342, 100684, 108974, 211500, 132600, 211700, 234800, 252800, 264600, 286000, 303200, 320500, 99609, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{12 , 0, 10196, 20391, 30586, 40782, 49805, 60000, 70196, 80391, 90586, 99609, 110978, 211500, 132600, 211700, 234800, 252800, 264600, 286000, 303200, 320500, 99609, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{12 , 0, 9218, 20391, 31564, 40782, 49805, 59022, 70196, 79413, 90586, 101760, 110978, 211500, 132600, 211700, 234800, 252800, 264600, 286000, 303200, 320500, 99609, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{12 , 0, 8680, 19316, 29414, 38632, 49805, 58485, 69658, 78876, 88974, 99609, 108289, 211500, 132600, 211700, 234800, 252800, 264600, 286000, 303200, 320500, 99609, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{12 , 0, 7605, 19316, 29710, 38631, 50342, 57947, 69658, 78338, 88974, 100684, 108289, 211500, 132600, 211700, 234800, 252800, 264600, 286000, 303200, 320500, 99609, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{12 , 0, 1232, 18734, 19841, 53777, 54682, 54682, 68539, 68539, 70196, 104486, 104486, 211500, 132600, 211700, 234800, 252800, 264600, 286000, 303200, 320500, 99609, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{12 , 0, 18781, 20391, 34741, 35722, 49805, 0, 70196, 88436, 89873, 104936, 106243, 79245, 83774, 90566, 99623, 104151, 110943, 90586, 99609, 101955, 108631, 110978, 117654, 112933, 0, 0, 0, 0, 0, 0},
								{12 , 0, 8790, 20391, 32762, 38631, 49805, 57735, 70196, 79556, 88436, 100956, 108827, 120000, 132600, 211700, 234800, 252800, 264600, 286000, 303200, 320500, 99609, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{12 , 0, 13023, 13857, 37909, 38631, 49127, 49805, 70196, 82939, 84053, 98931, 99609, 120000, 132600, 211700, 234800, 252800, 264600, 286000, 303200, 320500, 99609, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{12 , 0, 0, 22300, 23117, 48850, 48850, 49805, 70196, 71145, 93313, 93449, 120000, 120000, 132600, 211700, 234800, 252800, 264600, 286000, 303200, 320500, 99609, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{10 , 0, 20391, 29751, 31564, 49805, 70196, 81369, 99609, 101760, 108827, 93449, 120000, 120000, 132600, 211700, 234800, 252800, 264600, 286000, 303200, 320500, 99609, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{12 , 0, 0, 20391, 29751, 31564, 59022, 60935, 70196, 89380, 90586, 108827, 109906, 120000, 132600, 211700, 234800, 252800, 264600, 286000, 303200, 320500, 99609, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{12 , 0, 8790, 9360, 38631, 40068, 59022, 60300, 70196, 78985, 79556, 108827, 110740, 120000, 132600, 211700, 234800, 252800, 264600, 286000, 303200, 320500, 99609, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{12 , 0, 18578, 20391, 35248, 35354, 49805, 50648, 70196, 84053, 85159, 102379, 102958, 120000, 132600, 211700, 234800, 252800, 264600, 286000, 303200, 320500, 99609, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{7 , 0, 9218, 13324, 49805, 70196, 77263, 81369, 70196, 84053, 85159, 102379, 102958, 120000, 132600, 211700, 234800, 252800, 264600, 286000, 303200, 320500, 99609, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{12 , 0, 9022, 20391, 29414, 38631, 49805, 59022, 70196, 79218, 90586, 99609, 108827, 120000, 132600, 211700, 234800, 252800, 264600, 286000, 303200, 320500, 99609, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{22 , 0, 8658, 11054, 19189, 20321, 29651, 31247, 39011, 41524, 51225, 52635, 59964, 62192, 70828, 79856, 82632, 89195, 90704, 100558, 102673, 109881, 111886, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{12 , 0, 13324, 20391, 31564, 39435, 49805, 62361, 70196, 85259, 90586, 101760, 109630, 62192, 70828, 79856, 82632, 89195, 90704, 100558, 102673, 109881, 111886, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{22 , 0, 7067, 11173, 18240, 20391, 26687, 31564, 38631, 43508, 49805, 55132, 58251, 61749, 70196, 76492, 81369, 88436, 90586, 96883, 101760, 108827, 112933, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{12 , 0, 11173, 20391, 31564, 38631, 49805, 59022, 70196, 81369, 90586, 101760, 108827, 61749, 70196, 76492, 81369, 88436, 90586, 96883, 101760, 108827, 112933, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{12 , 0, 9895, 20391, 31564, 39435, 49805, 59700, 70196, 80091, 90586, 101760, 109630, 61749, 70196, 76492, 81369, 88436, 90586, 96883, 101760, 108827, 112933, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{22 , 0, 9218, 11173, 18240, 20391, 29414, 31564, 38631, 40587, 49805, 59022, 60978, 68045, 70196, 79413, 81369, 88436, 90586, 99609, 101760, 108827, 110782, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{7 , 0, 15300, 31500, 55200, 70600, 84800, 105800, 38631, 40587, 49805, 59022, 60978, 68045, 70196, 79413, 81369, 88436, 90586, 99609, 101760, 108827, 110782, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{7 , 0, 14458, 28275, 54623, 67191, 77486, 102379, 38631, 40587, 49805, 59022, 60978, 68045, 70196, 79413, 81369, 88436, 90586, 99609, 101760, 108827, 110782, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{7 , 0, 13700, 44600, 57500, 68700, 82000, 109800, 38631, 40587, 49805, 59022, 60978, 68045, 70196, 79413, 81369, 88436, 90586, 99609, 101760, 108827, 110782, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{5 , 0, 27845, 56918, 74026, 104195, 82000, 109800, 38631, 40587, 49805, 59022, 60978, 68045, 70196, 79413, 81369, 88436, 90586, 99609, 101760, 108827, 110782, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{5 , 0, 20496, 47663, 73630, 100494, 82000, 109800, 38631, 40587, 49805, 59022, 60978, 68045, 70196, 79413, 81369, 88436, 90586, 99609, 101760, 108827, 110782, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{7 , 0, 23117, 49805, 70196, 93313, 120000, 143117, 38631, 40587, 49805, 59022, 60978, 68045, 70196, 79413, 81369, 88436, 90586, 99609, 101760, 108827, 110782, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{12 , 0, 0, 23117, 23117, 11173, 46235, 49805, 70196, 70196, 70196, 93313, 81369, 68045, 70196, 79413, 81369, 88436, 90586, 99609, 101760, 108827, 110782, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{12 , 0, 0, 20391, 26687, 38631, 49805, 55132, 70196, 70196, 96883, 96883, 108827, 68045, 70196, 79413, 81369, 88436, 90586, 99609, 101760, 108827, 110782, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{6 , 0, 33613, 49805, 70196, 102673, 103808, 55132, 70196, 70196, 96883, 96883, 108827, 68045, 70196, 79413, 81369, 88436, 90586, 99609, 101760, 108827, 110782, 106143, 108827, 112933, 0, 0, 0, 0, 0, 0},
								{24 , 0, 9022, 11369, 18045, 20391, 29414, 31760, 38436, 40782, 49805, 52151, 58827, 61173, 67849, 70196, 79218, 81564, 88240, 90586, 99609, 101955, 108631, 110978, 117654, 112933, 0, 0, 0, 0, 0, 0},
								{12 , 0, 6674, 19998, 25268, 36891, 50198, 56872, 70196, 87089, 95464, 100380, 107087, 61173, 67849, 70196, 79218, 81564, 88240, 90586, 99609, 101955, 108631, 110978, 117654, 112933, 0, 0, 0, 0, 0, 0},
								{17 , 0, 9895, 15064, 20391, 30286, 35455, 40782, 49805, 59700, 64868, 70196, 80091, 85259, 90586, 99609, 109504, 114673, 88240, 90586, 99609, 101955, 108631, 110978, 117654, 112933, 0, 0, 0, 0, 0, 0},
								{12 , 0, 30185, 19998, 26237, 36891, 50198, 58251, 70196, 85709, 87089, 98640, 107087, 85259, 90586, 99609, 109504, 114673, 88240, 90586, 99609, 101955, 108631, 110978, 117654, 112933, 0, 0, 0, 0, 0, 0},
								{12 , 0, 6600, 20200, 31600, 39900, 50900, 64000, 70600, 80300, 91000, 101100, 109200, 85259, 90586, 99609, 109504, 114673, 88240, 90586, 99609, 101955, 108631, 110978, 117654, 112933, 0, 0, 0, 0, 0, 0},
								{7 , 0, 20391, 38631, 51955, 70196, 88436, 101760, 91000, 101100, 109200, 85259, 90586, 99609, 109504, 114673, 88240, 90586, 99609, 101955, 108631, 110978, 117654, 112933, 0, 0, 0, 0, 0, 0},
								{7 , 0, 19700, 34100, 49500, 70300, 85300, 100900, 88436, 101760, 91000, 101100, 109200, 85259, 90586, 99609, 109504, 114673, 88240, 90586, 99609, 101955, 108631, 110978, 117654, 112933, 0, 0, 0, 0, 0, 0},
								{12 , 0, 23510, 19998, 30185, 35134, 50198, 56872, 70196, 84938, 90193, 100002, 110264, 85259, 90586, 99609, 109504, 114673, 88240, 90586, 99609, 101955, 108631, 110978, 117654, 112933, 0, 0, 0, 0, 0, 0},
								{12 , 0, 20391, 23117, 26687, 43508, 47078, 66626, 73765, 93313, 96883, 113704, 117274, 77263, 88436, 90586, 97654, 99609, 108827, 90586, 99609, 101955, 108631, 110978, 117654, 112933, 0, 0, 0, 0, 0, 0},
								{7 , 0, 17800, 33900, 44800, 66200, 88800, 110300, 70196, 84938, 90193, 100002, 105329, 85259, 90586, 99609, 109504, 114673, 88240, 90586, 99609, 101955, 108631, 110978, 117654, 112933, 0, 0, 0, 0, 0, 0},
								{18 , 0, 9057, 13585, 20377, 29434, 33962, 40755, 49811, 56604, 61132, 63396, 70189, 79245, 83774, 90566, 99623, 104151, 110943, 90586, 99609, 101955, 108631, 110978, 117654, 112933, 0, 0, 0, 0, 0, 0}
								};
										
								
								
								
								
								
								
uint32_t externalTuning[129];
uint32_t localTuningTemp[129];
uint16_t tuning8BitBuffer[768];

uint16_t tuningDACTable[128];


void loadTuning(void)
{
	tuningLoading = 1;
	if (tuning >= 1)
	{
		initiateLoadingTuningFromExternalMemory(tuning);
	}
	else
	{
		computeTuningDACTable(Local);
	}
}

void computeTuningDACTable(TuningLoadLocation local_or_external)
{
	for(int i = 0; i < 128; i++)
	{
		tuningDACTable[i] = calculateDACvalue(i, local_or_external);
	}
	tuningLoading = 0;
}

unsigned short calculateDACvalue(uint8_t noteVal, TuningLoadLocation local_or_external)
{
	uint32_t pitchclass;
	uint32_t templongnote = 0;
	uint32_t octavenum;
	uint32_t templongoctave;
	uint32_t DACval;
	uint32_t note;
	uint32_t cardinality;
	if (local_or_external == Local)
	{
		cardinality = factoryTunings[tuning][0];
	}
	else
	{
		cardinality = externalTuning[0];
	}
	pitchclass = (noteVal % cardinality);  //get the pitch class
	octavenum = noteVal / cardinality;  //get which octave it occurs in
	
	if (local_or_external == Local)
	{
		templongnote = ((factoryTunings[tuning][pitchclass + 1]  / 10) * scaledSemitoneDACvalue);
	}
	else
	{
		templongnote = ((externalTuning[pitchclass + 1]  / 10) * scaledSemitoneDACvalue);
	}
	
	templongnote = (templongnote / 100000);
	templongoctave = (octavenum * scaledOctaveDACvalue);
	templongoctave = (templongoctave / 100);
	DACval = templongnote + templongoctave;
	if (DACval > 65535)
	{
		DACval = 65535;
	}
	return (uint16_t)DACval;
}