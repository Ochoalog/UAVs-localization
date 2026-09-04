#include "PixelTransform.h"
#include "math.h"

PixelTransform::PixelTransform()
{
    // CALIBRATION POINTS - pixels - Lat/Lng
    //  1 -  878  331 - 30°04'01,12"S 51°07'17,49"W
    //  2 - 1250  703 - 30°04'02,63"S 51°07'15,73"W
    //  3 - 3114 2486 - 30°04'10,28"S 51°07'06,48"W
    //  4 -  950 1883 - 30°04'07,58"S 51°07'17,23"W
    //  5 - 3304   96 - 30°04'00,12"S 51°07'05,87"W
    //  6 - 1487   14 - 30°03'59,83"S 51°07'14,58"W
    //  7 -  165 1250 - 30°04'04,92"S 51°07'20,97"W
    //  8 - 1456 1788 - 30°04'07,18"S 51°07'14,76"W
    //  9 - 2122  738 - 30°04'02,77"S 51°07'11,53"W
    // 10 - 2419 2389 - 30°04'09,75"S 51°07'10,04"W
//    vector<Vector2i> pPts;
//    vector<Vector2d> wPts;
//    pPts.push_back(Vector2i( 878, 331)); wPts.push_back(Vector2d(Deg2Dec(30,04,01.12,'S'),Deg2Dec(51,07,17.49,'W')));
//    pPts.push_back(Vector2i(1250, 703)); wPts.push_back(Vector2d(Deg2Dec(30,04,02.63,'S'),Deg2Dec(51,07,15.73,'W')));
//    pPts.push_back(Vector2i(3114,2486)); wPts.push_back(Vector2d(Deg2Dec(30,04,10.28,'S'),Deg2Dec(51,07,06.48,'W')));
//    pPts.push_back(Vector2i( 950,1883)); wPts.push_back(Vector2d(Deg2Dec(30,04,07.58,'S'),Deg2Dec(51,07,17.23,'W')));
//    pPts.push_back(Vector2i(3304,  96)); wPts.push_back(Vector2d(Deg2Dec(30,04,00.12,'S'),Deg2Dec(51,07,05.87,'W')));
//    pPts.push_back(Vector2i(1487,  14)); wPts.push_back(Vector2d(Deg2Dec(30,03,59.83,'S'),Deg2Dec(51,07,14.58,'W')));
//    pPts.push_back(Vector2i( 165,1250)); wPts.push_back(Vector2d(Deg2Dec(30,04,04.92,'S'),Deg2Dec(51,07,20.97,'W')));
//    pPts.push_back(Vector2i(1456,1788)); wPts.push_back(Vector2d(Deg2Dec(30,04,07.18,'S'),Deg2Dec(51,07,14.76,'W')));
//    pPts.push_back(Vector2i(2122, 738)); wPts.push_back(Vector2d(Deg2Dec(30,04,02.77,'S'),Deg2Dec(51,07,11.53,'W')));
//    pPts.push_back(Vector2i(2419,2389)); wPts.push_back(Vector2d(Deg2Dec(30,04,09.75,'S'),Deg2Dec(51,07,10.04,'W')));

    // ARROIO DO MEIO
    //  1 -  3904  4082 - 29°24'19,92"S 51°56'23,04"W
    //  2 -  918   4131 - 29°24'07,49"S 51°56'42,61"W
    //  3 -  279   645  - 29°24'45,10"S 51°56'29,74"W
    //  4 -  3938  35   - 29°24'57,21"S 51°56'03,06"W
    //  5 -  3248  1108 - 29°24'00,33"S 51°56'12,72"W
    //  6 -  1615  2127 - 29°24'59,14"S 51°56'28,31"W
    //  7 -  2664  3349 - 29°24'10,49"S 51°56'27,49"W
    //  8 -  941   3775 - 29°24'05,57"S 51°56'40,72"W
    //  9 -  2066  2859 - 29°24'05,19"S 51°56'28,96"W
    // 10 -  1639  1497 - 29°24'55,69"S 51°56'25,08"W

    vector<Vector2i> pPts;
    vector<Vector2d> wPts;

    //MATHIAS' VERSION - ARROIO DO MEIO
//    pPts.push_back(Vector2i(1255,2633)); wPts.push_back(Vector2d(Deg2Dec(29,24,0.46,'S'),Deg2Dec(51,56,33.11,'W')));
//    pPts.push_back(Vector2i(564,3122)); wPts.push_back(Vector2d(Deg2Dec(29,24,0.29,'S'),Deg2Dec(51,56,39.99,'W')));
//    pPts.push_back(Vector2i(491,3638)); wPts.push_back(Vector2d(Deg2Dec(29,24,2.88,'S'),Deg2Dec(51,56,42.96,'W')));
//    pPts.push_back(Vector2i(939,4104)); wPts.push_back(Vector2d(Deg2Dec(29,24,7.42,'S'),Deg2Dec(51,56,42.34,'W')));
//    pPts.push_back(Vector2i(1892,4735)); wPts.push_back(Vector2d(Deg2Dec(29,24,15.03,'S'),Deg2Dec(51,56,39.25,'W')));
//    pPts.push_back(Vector2i(2699,4409)); wPts.push_back(Vector2d(Deg2Dec(29,24,16.63,'S'),Deg2Dec(51,56,32.43,'W')));
//    pPts.push_back(Vector2i(2966,3431)); wPts.push_back(Vector2d(Deg2Dec(29,24,12.24,'S'),Deg2Dec(51,56,25.93,'W')));
//    pPts.push_back(Vector2i(2945,2299)); wPts.push_back(Vector2d(Deg2Dec(29,24,5.76,'S'),Deg2Dec(51,56,20.54,'W')));
//    pPts.push_back(Vector2i(2158,1959)); wPts.push_back(Vector2d(Deg2Dec(29,24,0.50,'S'),Deg2Dec(51,56,23.98,'W')));
//    pPts.push_back(Vector2i(1503,1966)); wPts.push_back(Vector2d(Deg2Dec(29,23,57.76,'S'),Deg2Dec(51,56,28.25,'W')));
//    pPts.push_back(Vector2i(900,2223)); wPts.push_back(Vector2d(Deg2Dec(29,23,56.65,'S'),Deg2Dec(51,56,33.41,'W')));
//    pPts.push_back(Vector2i(962,2183)); wPts.push_back(Vector2d(Deg2Dec(29,23,56.60,'S'),Deg2Dec(51,56,32.80,'W')));
//    pPts.push_back(Vector2i(3244,4549)); wPts.push_back(Vector2d(Deg2Dec(29,24,19.70,'S'),Deg2Dec(51,56,29.40,'W')));
//    pPts.push_back(Vector2i(3866,2618)); wPts.push_back(Vector2d(Deg2Dec(29,24,11.50,'S'),Deg2Dec(51,56,16.00,'W')));

    //MATHIAS' VERSION - UFRGS VALE
//    30 04 42,89S   51 07 36,46W    - 798,4142
//    30 04 37,42S   51 07 35,53W    - 486,3126
//    30 04 28,61S   51 07 31,02W    - 380,2586
//    30 04 34,29S   51 07 22,33W    - 2146,2948
//    30 04 33,13S   51 07 17,60W    - 1756,3052
//    30 04 33,50S   51 07 18,38W    - 1668,3170
//    30 04 24,88S   51 07 15,40W    - 1532,2886
//    30 04 33,49S   51 07 06,17W    - 1300,3204
//    30 04 41,27S   51 07 10,17W    - 1336,3962
//    30 04 40,91S   51 07 19,33W    - 974,3226

    /*pPts.push_back(Vector2i(317,3800)); wPts.push_back(Vector2d(Deg2Dec(30,04,37.69,'S'),Deg2Dec(51,07,36.08,'W')));
    pPts.push_back(Vector2i(1044,786)); wPts.push_back(Vector2d(Deg2Dec(30,04,25.10,'S'),Deg2Dec(51,07,24.27,'W')));
    pPts.push_back(Vector2i(1631,171)); wPts.push_back(Vector2d(Deg2Dec(30,04,23.51,'S'),Deg2Dec(51,07,19.49,'W')));
    pPts.push_back(Vector2i(2844,1682)); wPts.push_back(Vector2d(Deg2Dec(30,04,33.38,'S'),Deg2Dec(51,07,16.80,'W')));
    pPts.push_back(Vector2i(2104,2311)); wPts.push_back(Vector2d(Deg2Dec(30,04,34.71,'S'),Deg2Dec(51,07,22.49,'W')));
    pPts.push_back(Vector2i(1812,2971)); wPts.push_back(Vector2d(Deg2Dec(30,04,37.15,'S'),Deg2Dec(51,07,25.77,'W')));
    pPts.push_back(Vector2i(1682,3372)); wPts.push_back(Vector2d(Deg2Dec(30,04,38.78,'S'),Deg2Dec(51,07,27.53,'W')));*/

//    pPts.push_back(Vector2i(2299,1917)); wPts.push_back(Vector2d(Deg2Dec(30,04,33.24,'S'),Deg2Dec(51,07,20.32,'W')));
//    pPts.push_back(Vector2i(1161,2235)); wPts.push_back(Vector2d(Deg2Dec(30,04,32.19,'S'),Deg2Dec(51,07,27.38,'W')));
//    pPts.push_back(Vector2i(1278,3049)); wPts.push_back(Vector2d(Deg2Dec(30,04,36.29,'S'),Deg2Dec(51,07,28.87,'W')));



    //Rostock
    /*
        1684,261         54,10,36.71 N   12,18,19.03 E
        4132,1892        54,10,42.46 N   12,19,01.07 E
        547,3546         54,10,10.53 N	 12,18,43.78 E
        2104,4465	     54,10,14.94 N 	 12,19,08.70 E
        4258,2951        54,10,36.99 N 	 12,19,13.63 E
        3168,1608	     54,10,38.06 N	 12,18,48.52 E
        2193,323	     54,10,39.55 N	 12,18,24.78 E
        2638,339	     54,10,42.27 N	 12,18,29.37 E
        2301,1257	     54,10,34.75 N	 12,18,36.02 E
        4610,4021	     54,10,32.95 N 	 12,19,28.34 E
    */

    //Rostock
    /*pPts.push_back(Vector2i(1684,261));
    pPts.push_back(Vector2i(4132,1892));
    pPts.push_back(Vector2i(547,3546));
    pPts.push_back(Vector2i(2104,4465));
    pPts.push_back(Vector2i(4258,2951));
    pPts.push_back(Vector2i(3168,1608));
    pPts.push_back(Vector2i(2193,323));
    pPts.push_back(Vector2i(2638,339));
    pPts.push_back(Vector2i(2301,1257));
    pPts.push_back(Vector2i(4610,4021));


    Vector2d* v   = new Vector2d(Vector2d(Deg2Dec(54,10,36.71, 'N'),Deg2Dec(12,18,19.03,'E')));
    Vector2d* v2  = new Vector2d(Vector2d(Deg2Dec(54,10,42.46, 'N'),Deg2Dec(12,19,01.07,'E')));
    Vector2d* v3  = new Vector2d(Vector2d(Deg2Dec(54,10,10.53, 'N'),Deg2Dec(12,18,43.78,'E')));
    Vector2d* v4  = new Vector2d(Vector2d(Deg2Dec(54,10,14.94, 'N'),Deg2Dec(12,19,08.70,'E')));
    Vector2d* v5  = new Vector2d(Vector2d(Deg2Dec(54,10,36.99, 'N'),Deg2Dec(12,19,13.63,'E')));
    Vector2d* v6  = new Vector2d(Vector2d(Deg2Dec(54,10,38.06, 'N'),Deg2Dec(12,18,48.52,'E')));
    Vector2d* v7  = new Vector2d(Vector2d(Deg2Dec(54,10,39.55, 'N'),Deg2Dec(12,18,24.78,'E')));
    Vector2d* v8  = new Vector2d(Vector2d(Deg2Dec(54,10,42.27, 'N'),Deg2Dec(12,18,29.37,'E')));
    Vector2d* v9  = new Vector2d(Vector2d(Deg2Dec(54,10,34.75, 'N'),Deg2Dec(12,18,36.02,'E')));
    Vector2d* v10 = new Vector2d(Vector2d(Deg2Dec(54,10,32.95, 'N'),Deg2Dec(12,19,28.34,'E')));

    wPts.push_back(*v);
    wPts.push_back(*v2);
    wPts.push_back(*v3);
    wPts.push_back(*v4);
    wPts.push_back(*v5);
    wPts.push_back(*v6);
    wPts.push_back(*v7);
    wPts.push_back(*v8);
    wPts.push_back(*v9);
    wPts.push_back(*v10);*/


//Rostock Início ====================================================
    //1992,1134	54,10,36.64 N 	12,18,36.96 E
    //4545,330	54,10,42.13 N	12,18,26.81 E
    //2718,3901	54,10,42.70 N	12,18,47.51 E
    //2445,2232	54,10,39.48 N	12,18,40.72 E
    //3723,1224	54,10,41.32 N 	12,18,32.92 E
    //4005,4407	54,10,46.84 n	12,18,46.47 E
    /*pPts.push_back(Vector2i(1992,1134));
    pPts.push_back(Vector2i(4545,330));
    pPts.push_back(Vector2i(2718,3901));
    pPts.push_back(Vector2i(2445,2232));
    pPts.push_back(Vector2i(3723,1224));
    pPts.push_back(Vector2i(4005,4407));

    Vector2d* v = new Vector2d(Vector2d(Deg2Dec(54,10,36.64,'N'),Deg2Dec(12,18,36.96,'E')));
    Vector2d* v2 = new Vector2d(Vector2d(Deg2Dec(54,10,42.130,'N'),Deg2Dec(12,18,26.81,'E')));
    Vector2d* v3 = new Vector2d(Vector2d(Deg2Dec(54,10,42.70,'N'),Deg2Dec(12,18,47.51,'E')));
    Vector2d* v4 = new Vector2d(Vector2d(Deg2Dec(54,10,39.48,'N'),Deg2Dec(12,18,40.72,'E')));
    Vector2d* v5 = new Vector2d(Vector2d(Deg2Dec(54,10,41.32,'N'),Deg2Dec(12,18,32.92,'E')));
    Vector2d* v6 = new Vector2d(Vector2d(Deg2Dec(54,10,46.84,'N'),Deg2Dec(12,18,46.47,'E')));

    wPts.push_back(*v);
    wPts.push_back(*v2);
    wPts.push_back(*v3);
    wPts.push_back(*v4);
    wPts.push_back(*v5);
    wPts.push_back(*v6);*/
//====================================================================


    //Example Sequoia MSP ====================================================
        /*  905,380		46,31,28.09 N	6,33,03.37 E
            2469,298	46,31,27.12 N	6,33,11.49 E
            3180,396	46,31,26.19 N	6,33,15.03 E
            2550,1284	46,31,23.56 N	6,33,10.75 E
            4419,918	46,31,23.36 N	6,33,20.76 E
            4424,2176	46,31,18.91 N	6,33,19.29 E
            3618,2946	46,31,18.07 N	6,33,14.65 E
            1370,3136	46,31,17.98 N 	6,33,02.56 E
            356,1393	46,31,24.94 N 	6,32,59.38 E
            1091,1355 	46,31,24.48 N 	6,33,03.19 E

        pPts.push_back(Vector2i(905,380	 ));
        pPts.push_back(Vector2i(2469,298 ));
        pPts.push_back(Vector2i(3180,396 ));
        pPts.push_back(Vector2i(2550,1284));
        pPts.push_back(Vector2i(4419,918 ));
        pPts.push_back(Vector2i(4424,2176));
        pPts.push_back(Vector2i(3618,2946));
        pPts.push_back(Vector2i(1370,3136));
        pPts.push_back(Vector2i(356,1393 ));
        pPts.push_back(Vector2i(1091,1355));


        Vector2d* v = new   Vector2d(Vector2d(Deg2Dec(46,31,28.09,'N'),Deg2Dec(6,33,03.37,'E')));
        Vector2d* v2 = new  Vector2d(Vector2d(Deg2Dec(46,31,27.12,'N'),Deg2Dec(6,33,11.49,'E')));
        Vector2d* v3 = new  Vector2d(Vector2d(Deg2Dec(46,31,26.19,'N'),Deg2Dec(6,33,15.03,'E')));
        Vector2d* v4 = new  Vector2d(Vector2d(Deg2Dec(46,31,23.56,'N'),Deg2Dec(6,33,10.75,'E')));
        Vector2d* v5 = new  Vector2d(Vector2d(Deg2Dec(46,31,23.36,'N'),Deg2Dec(6,33,20.76,'E')));
        Vector2d* v6 = new  Vector2d(Vector2d(Deg2Dec(46,31,18.91,'N'),Deg2Dec(6,33,19.29,'E')));
        Vector2d* v7 = new  Vector2d(Vector2d(Deg2Dec(46,31,18.07,'N'),Deg2Dec(6,33,14.65,'E')));
        Vector2d* v8 = new  Vector2d(Vector2d(Deg2Dec(46,31,17.98,'N'),Deg2Dec(6,33,02.56,'E')));
        Vector2d* v9 = new  Vector2d(Vector2d(Deg2Dec(46,31,24.94,'N'),Deg2Dec(6,32,59.38,'E')));
        Vector2d* v10 = new Vector2d(Vector2d(Deg2Dec(46,31,24.48,'N'),Deg2Dec(6,33,03.19,'E')));

        wPts.push_back(*v);
        wPts.push_back(*v2);
        wPts.push_back(*v3);
        wPts.push_back(*v4);
        wPts.push_back(*v5);
        wPts.push_back(*v6);
        wPts.push_back(*v7);
        wPts.push_back(*v8);
        wPts.push_back(*v9);
        wPts.push_back(*v10);*/
    //====================================================================

    //UFRGS VET ====================================================
        /*
            81,127		30,04,36.92 S	51,07,39.57 W
            3778,1397   30,04,29.27 S   51,07,37.04 W
            4308,3487	30,04,27.97 S	51,07,32.19 W
            798,4416	30,04,35.01 S	51,07,29.58 W
            2714,1495	30,04,31.42 S	51,07,36.67 W
            226,3036	30,04,36.32 S	51,07,32.75 W
            840,465		30,04,35.34 S	51,07,38.87 W
            2088,41		30,04,32.85 S	51,07,40.02 W
            3982,2960	30,04,28.69 S	51,07,33.39 W
            456,1228	30,04,36.04 S	51,07,37.03 W
        */

        /*pPts.push_back(Vector2i(81,127	 ));
        pPts.push_back(Vector2i(3778,1397));
        pPts.push_back(Vector2i(4308,3487));
        pPts.push_back(Vector2i(798,4416 ));
        pPts.push_back(Vector2i(2714,1495));
        pPts.push_back(Vector2i(226,3036 ));
        pPts.push_back(Vector2i(840,465	 ));
        pPts.push_back(Vector2i(2088,41	 ));
        pPts.push_back(Vector2i(3982,2960));
        pPts.push_back(Vector2i(456,1228 ));

        Vector2d* v = new   Vector2d(Vector2d(Deg2Dec(30,04,36.92, 'S'),Deg2Dec(51,07,39.57, 'W')));
        Vector2d* v2 = new   Vector2d(Vector2d(Deg2Dec(30,04,29.27, 'S'),Deg2Dec(51,07,37.04, 'W')));
        Vector2d* v3 = new   Vector2d(Vector2d(Deg2Dec(30,04,27.97, 'S'),Deg2Dec(51,07,32.19, 'W')));
        Vector2d* v4 = new   Vector2d(Vector2d(Deg2Dec(30,04,35.01, 'S'),Deg2Dec(51,07,29.58, 'W')));
        Vector2d* v5 = new   Vector2d(Vector2d(Deg2Dec(30,04,31.42, 'S'),Deg2Dec(51,07,36.67, 'W')));
        Vector2d* v6 = new   Vector2d(Vector2d(Deg2Dec(30,04,36.32, 'S'),Deg2Dec(51,07,32.75, 'W')));
        Vector2d* v7 = new   Vector2d(Vector2d(Deg2Dec(30,04,35.34, 'S'),Deg2Dec(51,07,38.87, 'W')));
        Vector2d* v8 = new   Vector2d(Vector2d(Deg2Dec(30,04,32.85, 'S'),Deg2Dec(51,07,40.02, 'W')));
        Vector2d* v9 = new   Vector2d(Vector2d(Deg2Dec(30,04,28.69, 'S'),Deg2Dec(51,07,33.39, 'W')));
        Vector2d* v10 = new   Vector2d(Vector2d(Deg2Dec(30,04,36.04, 'S'),Deg2Dec(51,07,37.03, 'W')));


        wPts.push_back(*v);
        wPts.push_back(*v2);
        wPts.push_back(*v3);
        wPts.push_back(*v4);
        wPts.push_back(*v5);
        wPts.push_back(*v6);
        wPts.push_back(*v7);
        wPts.push_back(*v8);
        wPts.push_back(*v9);
        wPts.push_back(*v10);*/
    //====================================================================


    //UFRGS VET V3 ====================================================
        /*
        126,275		30,04,38.05 S	51,07,42.88 W
        2304,1055	30,04.31.03 S 	51,07,40.27 W
        2829,1950	30,04,29.27 S	51,07,37.03 W
        1815,2152	30,04,32.47 S	51,07,36.17 W
        357,2682	30,04,37.06 S	51,07,34.05 W
        2330,3963 	30,04,30.65 S	51,07,29.57 W
        553,4135	30,04,36.29 S	51,07,28.72 W
        1123,2797	30,04,34.61 S	51,07,33.71 W
        2420,1340	30,04,30.63 S	51,07,39.23 W
        */

        /*pPts.push_back(Vector2i(126,275	 ));
        pPts.push_back(Vector2i(2304,1055));
        pPts.push_back(Vector2i(2829,1950));
        pPts.push_back(Vector2i(1815,2152));
        pPts.push_back(Vector2i(357,2682 ));
        pPts.push_back(Vector2i(2330,3963));
        pPts.push_back(Vector2i(553,4135 ));
        pPts.push_back(Vector2i(1123,2797));
        pPts.push_back(Vector2i(2420,1340));

        Vector2d* v = new    Vector2d(Vector2d(Deg2Dec(30,04,38.05, 'S'),Deg2Dec(51,07,42.88, 'W')));
        Vector2d* v3 = new   Vector2d(Vector2d(Deg2Dec(30,04,31.03, 'S'),Deg2Dec(51,07,40.27, 'W')));
        Vector2d* v4 = new   Vector2d(Vector2d(Deg2Dec(30,04,29.27, 'S'),Deg2Dec(51,07,37.03, 'W')));
        Vector2d* v5 = new   Vector2d(Vector2d(Deg2Dec(30,04,32.47, 'S'),Deg2Dec(51,07,36.17, 'W')));
        Vector2d* v6 = new   Vector2d(Vector2d(Deg2Dec(30,04,37.06, 'S'),Deg2Dec(51,07,34.05, 'W')));
        Vector2d* v7 = new   Vector2d(Vector2d(Deg2Dec(30,04,30.65, 'S'),Deg2Dec(51,07,29.57, 'W')));
        Vector2d* v8 = new   Vector2d(Vector2d(Deg2Dec(30,04,36.29, 'S'),Deg2Dec(51,07,28.72, 'W')));
        Vector2d* v9 = new   Vector2d(Vector2d(Deg2Dec(30,04,34.61, 'S'),Deg2Dec(51,07,33.71, 'W')));
        Vector2d* v10 = new  Vector2d(Vector2d(Deg2Dec(30,04,30.63, 'S'),Deg2Dec(51,07,39.23, 'W')));


        wPts.push_back(*v);
        wPts.push_back(*v3);
        wPts.push_back(*v4);
        wPts.push_back(*v5);
        wPts.push_back(*v6);
        wPts.push_back(*v7);
        wPts.push_back(*v8);
        wPts.push_back(*v9);
        wPts.push_back(*v10);*/
    //====================================================================


    //ADM MATRICE 100 SEQUOIA 2019 ====================================================
        /*
            Imagem (x,y)	Google Earth(grau, min, sec, H)
            1771,358	29,23,59.95 S	51,56,09.88 W
            1600,319	29,23,58.93 S	51,56,09.72 W
            768,352		29,23,54.09 S	51,56,10.43 W
            3044,835	29,24,07.63 S	51,56,12.32 W
            2175,1121	29,24,02.70 S	51,56,14.76 W
            1798,1764	29,24,00.84 S	51,56,19.29 W
            4380,4587	29,24,17.37 S	51,56,36.68 W
            2709,4581	29,24,07.61 S	51,56,37.63 W
            1884,4535	29,24,02.77 S	51,56,37.81 W
            978,4078	29,23,57.23 S	51,56,35.28 W
            394,1214 	29,23,52.35 S	51,56,16.43 W
        */

        /*pPts.push_back(Vector2i(1771,358 ));
        pPts.push_back(Vector2i(1600,319 ));
        pPts.push_back(Vector2i(768,352	 ));
        pPts.push_back(Vector2i(3044,835 ));
        pPts.push_back(Vector2i(2175,1121));
        pPts.push_back(Vector2i(1798,1764));
        pPts.push_back(Vector2i(4380,4587));
        pPts.push_back(Vector2i(2709,4581));
        pPts.push_back(Vector2i(1884,4535));
        pPts.push_back(Vector2i(978,4078 ));
        pPts.push_back(Vector2i(394,1214 ));


        Vector2d* v = new     Vector2d(Vector2d(Deg2Dec(29,23,59.95, 'S'),Deg2Dec(51,56,09.88, 'W')));
        Vector2d* v2 = new    Vector2d(Vector2d(Deg2Dec(29,23,58.93, 'S'),Deg2Dec(51,56,09.72, 'W')));
        Vector2d* v3 = new    Vector2d(Vector2d(Deg2Dec(29,23,54.09, 'S'),Deg2Dec(51,56,10.43, 'W')));
        Vector2d* v4 = new    Vector2d(Vector2d(Deg2Dec(29,24,07.63, 'S'),Deg2Dec(51,56,12.32, 'W')));
        Vector2d* v5 = new    Vector2d(Vector2d(Deg2Dec(29,24,02.70, 'S'),Deg2Dec(51,56,14.76, 'W')));
        Vector2d* v6 = new    Vector2d(Vector2d(Deg2Dec(29,24,00.84, 'S'),Deg2Dec(51,56,19.29, 'W')));
        Vector2d* v7 = new    Vector2d(Vector2d(Deg2Dec(29,24,17.37, 'S'),Deg2Dec(51,56,36.68, 'W')));
        Vector2d* v8 = new    Vector2d(Vector2d(Deg2Dec(29,24,07.61, 'S'),Deg2Dec(51,56,37.63, 'W')));
        Vector2d* v9 = new    Vector2d(Vector2d(Deg2Dec(29,24,02.77, 'S'),Deg2Dec(51,56,37.81, 'W')));
        Vector2d* v10 = new   Vector2d(Vector2d(Deg2Dec(29,23,57.23, 'S'),Deg2Dec(51,56,35.28, 'W')));
        Vector2d* v11 = new   Vector2d(Vector2d(Deg2Dec(29,23,52.35, 'S'),Deg2Dec(51,56,16.43, 'W')));

        wPts.push_back(*v);
        wPts.push_back(*v2);
        wPts.push_back(*v3);
        wPts.push_back(*v4);
        wPts.push_back(*v5);
        wPts.push_back(*v6);
        wPts.push_back(*v7);
        wPts.push_back(*v8);
        wPts.push_back(*v9);
        wPts.push_back(*v10);
        wPts.push_back(*v11);*/
    //====================================================================


    //ADM MATRICE 100 SEQUOIA 2019 ADM 040 Mapas V2 ====================================================
        /*
            Imagem (x,y)	Google Earth(grau, min, sec, H)
        Imagem (x,y)	Google Earth(grau, min, sec, H)
        1402,2537	29,24,00.59 S	51,56,47.83 W
        475,4788	29,24,11.11 S	51,57,12.79 W
        24,1906		29,23,46.72 S	51,56,54.76 W
        1341,724	29,23,46.66 S	51,56,34.47 W
        3823,283	29,23,59.95 S	51,56,09.88 W
        2516,1577	29,24,00.87 S	51,56,30.99 W
        3752,3239	29,24,21.43 S	51,56,33.21 W
        2413,2762	29,24,08.99 S	51,56,40.95 W
        4191,2393	29,24,18.08 S	51,56,22.99 W
        3769,3729	29,24,25.15 S	51,56,36.80 W


        pPts.push_back(Vector2i(1402,2537));
        pPts.push_back(Vector2i(475,4788 ));
        pPts.push_back(Vector2i(24,1906	 ));
        pPts.push_back(Vector2i(1341,724 ));
        pPts.push_back(Vector2i(3823,283 ));
        pPts.push_back(Vector2i(2516,1577));
        pPts.push_back(Vector2i(3752,3239));
        pPts.push_back(Vector2i(2413,2762));
        pPts.push_back(Vector2i(4191,2393));
        pPts.push_back(Vector2i(3769,3729));

        Vector2d* v = new     Vector2d(Vector2d(Deg2Dec(29,24,00.59, 'S'),Deg2Dec(51,56,47.83, 'W')));
        Vector2d* v2 = new    Vector2d(Vector2d(Deg2Dec(29,24,11.11, 'S'),Deg2Dec(51,57,12.79, 'W')));
        Vector2d* v3 = new    Vector2d(Vector2d(Deg2Dec(29,23,46.72, 'S'),Deg2Dec(51,56,54.76, 'W')));
        Vector2d* v4 = new    Vector2d(Vector2d(Deg2Dec(29,23,46.66, 'S'),Deg2Dec(51,56,34.47, 'W')));
        Vector2d* v5 = new    Vector2d(Vector2d(Deg2Dec(29,23,59.95, 'S'),Deg2Dec(51,56,09.88, 'W')));
        Vector2d* v6 = new    Vector2d(Vector2d(Deg2Dec(29,24,00.87, 'S'),Deg2Dec(51,56,30.99, 'W')));
        Vector2d* v7 = new    Vector2d(Vector2d(Deg2Dec(29,24,21.43, 'S'),Deg2Dec(51,56,33.21, 'W')));
        Vector2d* v8 = new    Vector2d(Vector2d(Deg2Dec(29,24,08.99, 'S'),Deg2Dec(51,56,40.95, 'W')));
        Vector2d* v9 = new    Vector2d(Vector2d(Deg2Dec(29,24,18.08, 'S'),Deg2Dec(51,56,22.99, 'W')));
        Vector2d* v10 = new   Vector2d(Vector2d(Deg2Dec(29,24,25.15, 'S'),Deg2Dec(51,56,36.80, 'W')));

        wPts.push_back(*v);
        wPts.push_back(*v2);
        wPts.push_back(*v3);
        wPts.push_back(*v4);
        wPts.push_back(*v5);
        wPts.push_back(*v6);
        wPts.push_back(*v7);
        wPts.push_back(*v8);
        wPts.push_back(*v9);
        wPts.push_back(*v10);*/

    //====================================================================

        //ADM MATRICE 100 SEQUOIA 2019 ADM 040 Mapas V3 ====================================================
            /*
                Imagem (x,y)	Google Earth(grau, min, sec, H)
                1959,1611	29,24,04.47 S	51,56,20.65 W
                1215,92		29,23,54.20 S	51,56,13.43 W
                144,2113	29,23,55.51 S	51,56,32.04 W
                156,4714	29,24,05.50 S	51,56,49.84 W
                1231,4278	29,24,10.28 S	51,56,42.16 W
                946,4533	29,24,09.54 S	51,56,45.15 W
                3468,4152	29,24,23.19 S	51,56,31.52 W
                2195,2328 	29,24,08.63 S	51,56,24.56 W
                1236,1473	29,23,59.61 S	51,56,22.86 W
                3592,2615	29,24,18.09 S	51,56,20.41 W
            */

            pPts.push_back(Vector2i(1959,1611));
            pPts.push_back(Vector2i(1215,92	 ));
            pPts.push_back(Vector2i(144,2113 ));
            pPts.push_back(Vector2i(156,4714 ));
            pPts.push_back(Vector2i(1231,4278));
            pPts.push_back(Vector2i(946,4533 ));
            pPts.push_back(Vector2i(3468,4152));
            pPts.push_back(Vector2i(2195,2328));
            pPts.push_back(Vector2i(1236,1473));
            pPts.push_back(Vector2i(3592,2615));

            Vector2d* v = new     Vector2d(Vector2d(Deg2Dec(29,24,04.47, 'S'),Deg2Dec(51,56,20.65, 'W')));
            Vector2d* v2 = new    Vector2d(Vector2d(Deg2Dec(29,23,54.20, 'S'),Deg2Dec(51,56,13.43, 'W')));
            Vector2d* v3 = new    Vector2d(Vector2d(Deg2Dec(29,23,55.51, 'S'),Deg2Dec(51,56,32.04, 'W')));
            Vector2d* v4 = new    Vector2d(Vector2d(Deg2Dec(29,24,05.50, 'S'),Deg2Dec(51,56,49.84, 'W')));
            Vector2d* v5 = new    Vector2d(Vector2d(Deg2Dec(29,24,10.28, 'S'),Deg2Dec(51,56,42.16, 'W')));
            Vector2d* v6 = new    Vector2d(Vector2d(Deg2Dec(29,24,09.54, 'S'),Deg2Dec(51,56,45.15, 'W')));
            Vector2d* v7 = new    Vector2d(Vector2d(Deg2Dec(29,24,23.19, 'S'),Deg2Dec(51,56,31.52, 'W')));
            Vector2d* v8 = new    Vector2d(Vector2d(Deg2Dec(29,24,08.63, 'S'),Deg2Dec(51,56,24.56, 'W')));
            Vector2d* v9 = new    Vector2d(Vector2d(Deg2Dec(29,23,59.61, 'S'),Deg2Dec(51,56,22.86, 'W')));
            Vector2d* v10 = new   Vector2d(Vector2d(Deg2Dec(29,24,18.09, 'S'),Deg2Dec(51,56,20.41, 'W')));

            wPts.push_back(*v);
            wPts.push_back(*v2);
            wPts.push_back(*v3);
            wPts.push_back(*v4);
            wPts.push_back(*v5);
            wPts.push_back(*v6);
            wPts.push_back(*v7);
            wPts.push_back(*v8);
            wPts.push_back(*v9);
            wPts.push_back(*v10);



    // Convert wPts to meters
    for(unsigned int i=0; i<wPts.size(); ++i){
        UTMCoordinates utm;
        UTMConverter::latitudeAndLongitudeToUTMCoordinates(wPts[i][0],wPts[i][1],utm);
        cout << i
             << " Lat/Lng " <<  wPts[i][0] << ' ' << wPts[i][1]
             << " meters " << utm.northing << ' ' << utm.easting << endl;
        wPts[i][0] = utm.northing;
        wPts[i][1] = utm.easting;
    }

    // METERS -> PIXELS
    // FINDING PARAMETERS - TRANSLATION AND SCALE
    // x_p = (x_m - xo_m)/(xf_m - xo_m) * (xf_p - xo_p) = (x_m - xo_m) * alfa = x_m * a + b
    // y_p = (y_m - yo_m)/(yf_m - yo_m) * (yf_p - yo_p) = (y_m - yo_m) * ratioY = y_m * c + d
    // Linear System
    // [x_m1 1 0 0; 0 0 y_m1 1; x_m2 1 0 0; 0 0 y_m2 1; ...] [a; b; c; d]^T = [x_p1; y_p1; x_p2; y_p2; ...; x_pn; y_pn]^T
    // A * state = B
    // nx4 * 4x1 = nx1

//    int n=wPts.size();
//    MatrixXd A(2*n,4);
//    VectorXd state(4);
//    VectorXd b(2*n);

//    for(int l=0; l<n; ++l){
//        int i=2*l;
//        int j=2*l+1;
//        A(i,0) = wPts[l][1]; A(i,1) = 1; A(i,2) = 0;          A(i,3) = 0;
//        A(j,0) = 0;          A(j,1) = 0; A(j,2) = wPts[l][0]; A(j,3) = 1;
//        b(i) = pPts[l][0];   b(j) = pPts[l][1];
//    }

//    state = (A.transpose() * A).inverse() * (A.transpose() * b);

//    scaleX = state[0];
//    deltaX = state[1];
//    scaleY = state[2];
//    deltaY = state[3];


    //TRANSFORMATION - SRT
    // x_p = (x_m - xo_m)/(xf_m - xo_m) * (xf_p - xo_p) = (x_m - xo_m) * alfa = x_m * a + y_m * b + c
    // y_p = (y_m - yo_m)/(yf_m - yo_m) * (yf_p - yo_p) = (y_m - yo_m) * ratioY = x_m * c + y_m * d + f

    // Linear System
    // [x_m1 y_m1 1 0 0 0; 0 0 0 x_m1 y_m1 1; ...] [a; b; c; d; e; f]^T = [x_p1; y_p1; x_p2; y_p2; ...; x_pn; y_pn]^T
    // A * state = B
    // nx6 * 6x1 = nx1

    int n=wPts.size();
    MatrixXd A(2*n,6);
    VectorXd state(6);
    VectorXd b(2*n);

    for(int l=0; l<n; ++l){
        int i=2*l;
        int j=2*l+1;
        A(i,0) = wPts[l][1]; A(i,1) = wPts[l][0]; A(i,2) = 1; A(i,3) = 0; A(i,4) = 0; A(i,5) = 0;
        A(j,0) = 0;  A(j,1) = 0; A(j,2) = 0; A(j,3) = wPts[l][1]; A(j,4) = wPts[l][0]; A(j,5) = 1;
        b(i) = pPts[l][0];   b(j) = pPts[l][1];
    }

    // Solving the linear system
    // A^T * A * x = A^T * b
    // 4xn * nx4 * 4x1 = 4xn * nx1
    state = (A.transpose() * A).inverse() * (A.transpose() * b);

    transfA = state[0];
    transfB = state[1];
    transfC = state[2];
    transfD = state[3];
    transfE = state[4];
    transfF = state[5];


    //    scaleX = state[0];
    //    deltaX = state[1];
    //    scaleY = state[2];
    //    deltaY = state[3];

    scale = sqrt(transfA*transfA + transfC*transfC);

      VectorXd result = A*state;

        Mat img = imread("/home/phi/Documents/datasets/FOTOS-ADM-SEQUOIA-18_ABRIL/Imagens/0040/Mapas/testes/ADM_V3_05_09_2018.jpg",CV_LOAD_IMAGE_COLOR);

        //Draw Calibration points

        for(int l=0; l<n; ++l){
            Point2i center(pPts[l][0],pPts[l][1]);
            circle( img, center, 50, Scalar( 0, 0, 255 ), 5);
            circle( img, center, 5, Scalar( 0, 0, 255 ), 15);

            Point2i newCenter(result[2*l],result[2*l+1]);
            circle( img, newCenter, 50, Scalar( 0, 180, 255 ), 5);
            circle( img, newCenter, 5, Scalar( 0, 180, 255 ), 15);
        }
        cv::resize(img,img,Size(0,0),0.2,0.2);
        cv::imshow("Mapa",img);
        cv::waitKey();
}

cv::Point2i PixelTransform::UTMtoPixel(UTMCoordinates& utm)
{
    cv::Point2i p;
    p.x = utm.easting*transfA + transfB*utm.northing + transfC;
    p.y = utm.easting*transfD + transfE*utm.northing + transfF;
    return p;
}

double PixelTransform::Pix2Met (double val)
{
    return val/scale;
}

double PixelTransform::Pix2MetX (double val)
{
    return val/scaleX;
}

double PixelTransform::Pix2MetY (double val)
{
    return val/scaleY;
}

double PixelTransform::Met2Pix (double val)
{
    return val*scale;
}

double PixelTransform::Met2PixX (double val)
{
    return val*scaleX;
}

double PixelTransform::Met2PixY (double val)
{
    return val*scaleY;
}

double PixelTransform::Deg2Dec(double deg, double min, double sec, char hem)
{
    double coord = deg + min/60.0 + sec/3600.0;
    if(hem == 'W' || hem == 'S')
        return -coord;
    else
        return coord;
}

