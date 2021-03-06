.PHONY: all clean debug profile

CXXFLAGS += -Wall -pthread -std=c++11 -g
LDFLAGS += -pthread -std=c++11 -rdynamic

ASSEMBLE_BIN := ${BIN_DIR}/abruijn-assemble
REPEAT_BIN := ${BIN_DIR}/abruijn-repeat
POLISH_BIN := ${BIN_DIR}/abruijn-polish

profile: CXXFLAGS += -pg
profile: LDFLAGS += -pg
profile: release

release: CXXFLAGS += -D_LOG -O3 -DNDEBUG
release: abruijn-repeat abruijn-polish abruijn-assemble

debug: CXXFLAGS += -Og -D_GLIBCXX_DEBUG -D_DEBUG -D_DEBUG_LOG -D_LOG
debug: abruijn-repeat abruijn-polish abruijn-assemble


#sequence module
sequence_obj := ${patsubst %.cpp,%.o,${wildcard sequence/*.cpp}}

sequence/%.o: sequence/%.cpp sequence/*.h common/*h
	${CXX} -c ${CXXFLAGS} $< -o $@

#abruijn-assemble module
assemble_obj := ${patsubst %.cpp,%.o,${wildcard assemble/*.cpp}}
abruijn-assemble: ${assemble_obj} ${sequence_obj}
	${CXX} ${assemble_obj} ${sequence_obj} -o ${ASSEMBLE_BIN} ${LDFLAGS}

assemble/%.o: assemble/%.cpp assemble/*.h sequence/*.h common/*h
	${CXX} -c ${CXXFLAGS} $< -o $@

#abruijn-repeat module
repeat_obj := ${patsubst %.cpp,%.o,${wildcard repeat_graph/*.cpp}}
abruijn-repeat: ${repeat_obj} ${sequence_obj}
	${CXX} ${repeat_obj} ${sequence_obj} -o ${REPEAT_BIN} ${LDFLAGS}

repeat_graph/%.o: repeat_graph/%.cpp repeat_graph/*.h sequence/*.h common/*.h
	${CXX} -c ${CXXFLAGS} $< -o $@

#abruijn-polish module
polish_obj := ${patsubst %.cpp,%.o,${wildcard polishing/*.cpp}}
abruijn-polish: ${polish_obj}
	${CXX} ${polish_obj} -o ${POLISH_BIN} ${LDFLAGS}

polishing/%.o: polishing/%.cpp polishing/*.h common/*h
	${CXX} -c ${CXXFLAGS} $< -o $@



clean:
	-rm ${repeat_obj}
	-rm ${sequence_obj}
	-rm ${assemble_obj}
	-rm ${polish_obj}
	-rm ${REPEAT_BIN}
	-rm ${ASSEMBLE_BIN}
	-rm ${POLISH_BIN}
