TEMPLATE = app
CONFIG += console
CONFIG -= qt
CONFIG += console c++11
CONFIG -= app_bundle
#QMAKE_CXXFLAGS += -std=c++0x comentei e substitui pela de baixo (QMAKE_CXXFLAGS += -std=gnu++14), pois estava dando o seguinte erro: (unable to find numeric literal operator 'operator""Q')
QMAKE_CXXFLAGS += -std=gnu++14
QMAKE_CFLAGS_RELEASE += -fopenmp
QMAKE_CXXFLAGS += -fopenmp
QMAKE_LFLAGS += -fopenmp


SOURCES += \
    GlutClass.cpp \
    Grid.cpp \
    main.cpp \
    Robot.cpp \
    Utils.cpp \
    Mcl.cpp \
    DroneRobot.cpp \
    MapGrid.cpp \
    Kernel.cpp \
    SomeKernels.cpp \
    densityheuristic.cpp \
    ColorCPU.cpp \
    vec2.cpp \
    vec3.cpp \
    vec4.cpp \
    colorheuristic.cpp \
    Heuristic.cpp \
    miheuristic.cpp \
    MeanShiftHeuristic.cpp \
    SiftHeuristic.cpp \
    BriefHeuristic.cpp \
    DistanceTransform.cpp \
    CorrelativeSM.cpp \
    mutualinfheuristic.cpp \
    Ndvi.cpp \
    VegetationIndex.cpp \
    Grvi.cpp \
    VegetatonIndexGen.cpp

OTHER_FILES += \
    CONTROLE.txt

HEADERS += \
    Grid.h \
    GlutClass.h \
    Robot.h \
    Utils.h \
    Mcl.h \
    DroneRobot.h \
    MapGrid.h \
    Heuristic.h \
    Kernel.h \
    SomeKernels.h \
    ColorCPU.h \
    densityheuristic.h \
    mat3x3.h \
    vec2.h \
    vec3.h \
    vec4.h \
    RadiusVolumeTransferFunctions.h \
    colorheuristic.h \
    miheuristic.h \
    MeanShiftHeuristic.h \
    SiftHeuristic.h \
    BriefHeuristic.h \
    DistanceTransform.h \
    CorrelativeSM.h \
    mutualinfheuristic.h \
    Ndvi.h \
    VegetationIndex.h \
    config.h \
    Grvi.h \
    VegetatonIndexGen.h

INCLUDEPATH += /usr/local/Aria/include
INCLUDEPATH += -I/usr/local/include/opencv2
INCLUDEPATH += /usr/include/pcl-1.7
INCLUDEPATH += /usr/include/eigen3
INCLUDEPATH += /usr/local/include/pcl-1.9

LIBS += -L/usr/local/lib -L/usr/local/Aria/lib/ -lAria -lpthread -lglut -lGLEW -ldl -lrt `pkg-config opencv --libs` -lGL -lfreeimage -lboost_system -lopencv_ximgproc
LIBS += -lboost_system -lpcl_common -lpcl_io -lpcl_search -lpcl_kdtree -lpcl_registration

INCLUDEPATH += $$PWD/../../../../../usr/local/include
DEPENDPATH += $$PWD/../../../../../usr/local/include


win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../../usr/local/lib/release/ -lopencv_xfeatures2d
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../../usr/local/lib/debug/ -lopencv_xfeatures2d
else:unix: LIBS += -L$$PWD/../../../../../usr/local/lib/ -lopencv_xfeatures2d

INCLUDEPATH += $$PWD/../../../../../usr/local/include
DEPENDPATH += $$PWD/../../../../../usr/local/include




win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../../usr/local/lib/release/ -lopencv_features2d
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../../usr/local/lib/debug/ -lopencv_features2d
else:unix: LIBS += -L$$PWD/../../../../../usr/local/lib/ -lopencv_features2d

INCLUDEPATH += $$PWD/../../../../../usr/local/include
DEPENDPATH += $$PWD/../../../../../usr/local/include
