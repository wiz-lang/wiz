ifdef CC
ifdef CXX
	HAS_PREDEFINED_CC := 1
endif
endif

ifndef PREFIX
	PREFIX := /usr/local
endif

ifeq ($(OS),Windows_NT)
	EXE := .exe
	LOCATE_COMMAND := where
else
	EXE :=
	LOCATE_COMMAND := command -v
endif

HAS_CC := $(shell $(LOCATE_COMMAND) cc)
HAS_GCC := $(shell $(LOCATE_COMMAND) gcc)
HAS_CLANG := $(shell $(LOCATE_COMMAND) clang)

ifndef HAS_PREDEFINED_CC
ifdef HAS_CC
	CC := cc
	CXX := c++
else ifdef HAS_GCC
	CC := gcc
	CXX := g++
else ifdef HAS_CLANG
	CC := clang
	CXX := clang++
endif
endif

WIZ_SRC := src
WIZ_OUT_DIR := bin
WIZ_TEST_DIR := tests
WIZ_TEST_TMP_DIR := bin/test-tmp

WIZ_H_MATCH := $(wildcard $(WIZ_SRC)/wiz/*.h $(WIZ_SRC)/wiz/ast/*.h $(WIZ_SRC)/wiz/compiler/*.h $(WIZ_SRC)/wiz/parser/*.h  $(WIZ_SRC)/wiz/utility/*.h $(WIZ_SRC)/wiz/definition/*.h $(WIZ_SRC)/wiz/platform/*.h $(WIZ_SRC)/wiz/format/*.h)
WIZ_CPP_MATCH := $(wildcard $(WIZ_SRC)/wiz/*.cpp $(WIZ_SRC)/wiz/ast/*.cpp $(WIZ_SRC)/wiz/compiler/*.cpp $(WIZ_SRC)/wiz/parser/*.cpp  $(WIZ_SRC)/wiz/utility/*.cpp $(WIZ_SRC)/wiz/definition/*.cpp $(WIZ_SRC)/wiz/platform/*.cpp $(WIZ_SRC)/wiz/format/*.cpp)
WIZ_H_EXCLUDE := 
WIZ_CPP_EXCLUDE := 

WIZ_H := $(filter-out $(WIZ_H_EXCLUDE), $(WIZ_H_MATCH))
WIZ_CPP := $(filter-out $(WIZ_CPP_EXCLUDE), $(WIZ_CPP_MATCH))
WIZ_O := $(patsubst %.cpp, %.o, $(WIZ_CPP))
WIZ_DEPS := $(sort $(patsubst %.o, %.d, $(WIZ_O)))

ifndef PLATFORM
	PLATFORM := native
endif

ifndef CFG
	CFG := release
endif

ifndef WERR
	WERR := 1
endif

ifeq ($(WERR),0)
	WERR_ :=
else ifeq ($(WERR),1)
	WERR_ := -Werror -Wno-error=unused-variable -Wno-error=unused-result -Wno-error=unused-parameter -Wno-error=unused-function -Wno-error=unused-value
else ifeq ($(WERR),2)
	WERR_ := -Werror
else
$(error Unknown WERR setting '$(WERR)' (must be 0, 1, or 2))
endif

ifeq ($(PLATFORM),native)
ifeq ($(CFG),release)
	CXX_FLAGS := -D_POSIX_SOURCE -Os -std=c++17 -MMD -Wall -Wextra $(WERR_) -Wold-style-cast -Wnon-virtual-dtor -fno-exceptions -fno-rtti
else ifeq ($(CFG),debug)
	CXX_FLAGS := -D_POSIX_SOURCE -DWIZ_DEBUG -g -std=c++17 -MMD -Wall -Wextra $(WERR_) -Wold-style-cast -Wnon-virtual-dtor -fno-exceptions -fno-rtti
endif
	LXXFLAGS := -lm
	INCLUDES := -I$(WIZ_SRC)
	WIZ := wiz$(EXE)
else ifeq ($(PLATFORM),emcc)
	WIZ_PRE_JS := $(WIZ_SRC)/wiz-emscripten/pre-js.js
	WIZ := wiz.js
	CC := emcc
	CXX := em++
	CXX_FLAGS := -Oz -std=c++1z -MMD -Wall -Wextra $(WERR_) -Wold-style-cast -Wnon-virtual-dtor -fno-exceptions
	LXXFLAGS := -lm --bind --memory-init-file 0 -s NO_FILESYSTEM=1 -s INLINING_LIMIT=1 -s DISABLE_EXCEPTION_CATCHING=1 -s WASM=0 --pre-js $(WIZ_PRE_JS)
	INCLUDES := -I$(WIZ_SRC)
else
$(error Unknown PLATFORM value "$(PLATFORM)")
endif

.PHONY: clean all install
	
all: $(WIZ_OUT_DIR) $(WIZ_OUT_DIR)/$(WIZ)

$(WIZ_OUT_DIR):
	mkdir $(WIZ_OUT_DIR)

$(WIZ_TEST_TMP_DIR):
	mkdir $(WIZ_TEST_TMP_DIR)

$(WIZ_O): %.o: %.cpp
	$(CXX) $(CXX_FLAGS) -c -o $@ $< $(INCLUDES)

$(WIZ_OUT_DIR)/$(WIZ): $(WIZ_O)
	$(CXX) $(CXX_FLAGS) $^ $(LXXFLAGS) -o $@

clean:
	rm -f $(WIZ_OUT_DIR)/$(WIZ) $(WIZ_O) $(WIZ_DEPS)

install: $(WIZ_OUT_DIR)/$(WIZ)
	install -d $(DESTDIR)$(PREFIX)/bin/
	install -m 755 $(WIZ_OUT_DIR)/$(WIZ) $(DESTDIR)$(PREFIX)/bin/

tests: $(WIZ_OUT_DIR)/$(WIZ) $(WIZ_OUT_DIR) $(WIZ_TEST_TMP_DIR)
	$(WIZ_TEST_DIR)/wiztests.sh -w $(WIZ_OUT_DIR)/$(WIZ) -b $(WIZ_TEST_TMP_DIR) $(WIZ_TEST_DIR)/block $(WIZ_TEST_DIR)/failure

block-tests: $(WIZ_OUT_DIR)/$(WIZ) $(WIZ_TEST_TMP_DIR)
	$(WIZ_TEST_DIR)/wiztests.sh -w $(WIZ_OUT_DIR)/$(WIZ) -b $(WIZ_TEST_TMP_DIR) $(WIZ_TEST_DIR)/block$(TEST_NAME:%=/%.wiz)

failure-tests: $(WIZ_OUT_DIR)/$(WIZ) $(WIZ_TEST_TMP_DIR)
	$(WIZ_TEST_DIR)/wiztests.sh -w $(WIZ_OUT_DIR)/$(WIZ) -b $(WIZ_TEST_TMP_DIR) $(WIZ_TEST_DIR)/failure$(TEST_NAME:%=/%.wiz)


-include $(WIZ_DEPS)