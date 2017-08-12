AM_CPPFLAGS = -I$(srcdir)/../../third_party/decklink -I$(srcdir)/../util -I$(srcdir)/../display -I$(srcdir)/../scanner -I$(srcdir)/../barcoder -I$(srcdir)/../util $(XCBPRESENT_CFLAGS) $(XCB_CFLAGS) $(CXX14_FLAGS)
AM_CXXFLAGS = $(PICKY_CXXFLAGS)

bin_PROGRAMS = ps4_degrader test

ps4_degrader_SOURCES = Capture.cc Capture.hh Config.hh Config.cc Playback.cc Playback.hh h264_degrader.cc
ps4_degrader_LDADD = ../../third_party/decklink/libdecklink.a ../util/libutil.a $(XCBPRESENT_LIBS) $(XCB_LIBS) $(PANGOCAIRO_LIBS) $(AVFORMAT_LIBS) $(AVCODEC_LIBS) $(AVUTIL_LIBS) $(AVFILTER_LIBS) $(AVDEVICE_LIBS)
ps4_degrader_LDFLAGS = -pthread -ldl -lm

test_SOURCES = test.cc h264_degrader.cc
test_LDADD = ../../third_party/decklink/libdecklink.a ../util/libutil.a $(XCBPRESENT_LIBS) $(XCB_LIBS) $(PANGOCAIRO_LIBS) $(AVFORMAT_LIBS) $(AVCODEC_LIBS) $(AVUTIL_LIBS) $(AVFILTER_LIBS) $(AVDEVICE_LIBS)
test_LDFLAGS = -pthread -ldl -lm
