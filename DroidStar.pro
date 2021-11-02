QT += quick quickcontrols2 network multimedia
android:QT += androidextras
linux:QT += serialport
CONFIG += c++11
LFLAGS +=
android:INCLUDEPATH += $$(HOME)/Android/android-build/include
LIBS += -limbe_vocoder -ldl
macx:QT += serialport
macx::INCLUDEPATH += /usr/local/include
macx:LIBS += -L/usr/local/lib -framework AVFoundation
macx:QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.14
macx:QMAKE_INFO_PLIST = Info.plist
ios:LIBS += -framework AVFoundation
ios:QMAKE_INFO_PLIST = Info.plist
VERSION_BUILD='$(shell cd $$PWD;git rev-parse --short HEAD)'
DEFINES += VERSION_NUMBER=\"\\\"$${VERSION_BUILD}\\\"\"
DEFINES += QT_DEPRECATED_WARNINGS

#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
#DEFINES += USE_FLITE
DEFINES += AMBESW_SUPPORTED

SOURCES += \
        CRCenc.cpp \
        DMRData.cpp \
        Golay24128.cpp \
        SHA256.cpp \
        YSFConvolution.cpp \
        YSFFICH.cpp \
        androidserialport.cpp \
        audioengine.cpp \
        cbptc19696.cpp \
        cgolay2087.cpp \
        chamming.cpp \
        codec.cpp \
        codec2/codebooks.cpp \
        codec2/codec2.cpp \
        codec2/kiss_fft.cpp \
        codec2/lpc.cpp \
        codec2/nlp.cpp \
        codec2/pack.cpp \
        codec2/qbase.cpp \
        codec2/quantise.cpp \
        crs129.cpp \
        dcscodec.cpp \
        dmrcodec.cpp \
        droidstar.cpp \
        httpmanager.cpp \
        iaxcodec.cpp \
        m17codec.cpp \
        main.cpp \
        nxdncodec.cpp \
        p25codec.cpp \
        refcodec.cpp \
        serialambe.cpp \
        serialmodem.cpp \
        xrfcodec.cpp \
        ysfcodec.cpp
macx:OBJECTIVE_SOURCES += micpermission.mm
ios:OBJECTIVE_SOURCES += micpermission.mm
RESOURCES += qml.qrc

QML_IMPORT_PATH =
QML_DESIGNER_IMPORT_PATH =

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
	android/AndroidManifest.xml \
	android/build.gradle \
	android/gradle/wrapper/gradle-wrapper.jar \
	android/gradle/wrapper/gradle-wrapper.properties \
	android/gradlew \
	android/gradlew.bat \
	android/res/values/libs.xml \
	images/log.png

contains(ANDROID_TARGET_ARCH,armeabi-v7a) {
	LIBS += -L$$(HOME)/Android/local/lib
	ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
	OTHER_FILES += android/src
}

contains(ANDROID_TARGET_ARCH,arm64-v8a) {
	LIBS += -L$$(HOME)/Android/local/lib64
	ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
	OTHER_FILES += android/src
}

ANDROID_ABIS = armeabi-v7a arm64-v8a

HEADERS += \
	CRCenc.h \
	DMRData.h \
	DMRDefines.h \
	Golay24128.h \
	SHA256.h \
	YSFConvolution.h \
	YSFFICH.h \
	androidserialport.h \
	audioengine.h \
	cbptc19696.h \
	cgolay2087.h \
	chamming.h \
	codec.h \
	codec2/codec2.h \
	codec2/codec2_internal.h \
	codec2/defines.h \
	codec2/kiss_fft.h \
	codec2/lpc.h \
	codec2/nlp.h \
	codec2/qbase.h \
	codec2/quantise.h \
	crs129.h \
	dcscodec.h \
	dmrcodec.h \
	droidstar.h \
	httpmanager.h \
	iaxcodec.h \
	iaxdefines.h \
	m17codec.h \
	nxdncodec.h \
	p25codec.h \
	refcodec.h \
	serialambe.h \
	serialmodem.h \
	vocoder_plugin.h \
	xrfcodec.h \
	ysfcodec.h
macx:HEADERS += micpermission.h

contains(DEFINES, USE_FLITE){
	LIBS += -lflite_cmu_us_slt -lflite_cmu_us_kal16 -lflite_cmu_us_awb -lflite_cmu_us_rms -lflite_usenglish -lflite_cmulex -lflite -lasound
}
ios:HEADERS += micpermission.h