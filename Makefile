# See http://clang.llvm.org/cxx_status.html for clang++ details.
CXX_STANDARD=c++17


# By default, try to catch everything.
# This may be painful at first but it pays off in the long run.
COMMON_SANITISER_FLAGS=\
					   -fstack-protector-strong \
					   -fsanitize=undefined
CLANG_SANITISER_FLAGS=\
					  $(COMMON_SANITISER_FLAGS) \
					  -fsanitize=leak \
					  -fsanitize=address
GCC_SANITISER_FLAGS=\
					$(COMMON_SANITISER_FLAGS) \
					-fsanitize=leak


INCLUDE_FLAGS=\
			  -I./include
COMMON_CXXFLAGS=\
				-Wall \
				-Wextra \
				-Werror \
				-Wfatal-errors \
				-pedantic \
				-g \
				$(INCLUDE_FLAGS)
CLANG_CXXFLAGS=\
			   $(COMMON_CXXFLAGS) \
			   -Wabsolute-value \
			   -Wabstract-vbase-init \
			   -Warray-bounds-pointer-arithmetic \
			   -Wassign-enum \
			   -Wbad-function-cast \
			   -Wbitfield-enum-conversion \
			   -Wcast-align \
			   -Wcast-qual \
			   -Wchar-subscripts \
			   -Wcomma \
			   -Wconditional-uninitialized \
			   -Wconversion \
			   -Wconsumed \
			   -Wdate-time \
			   -Wdelete-non-virtual-dtor \
			   -Wdeprecated \
			   -Wdeprecated-implementations \
			   -Wdirect-ivar-access \
			   -Wdiv-by-zero \
			   -Wdouble-promotion \
			   -Wduplicate-enum \
			   -Wduplicate-method-arg \
			   -Wduplicate-method-match \
			   -Wfloat-conversion \
			   -Wint-to-void-pointer-cast \
			   -Wfor-loop-analysis \
			   -Wformat-nonliteral \
			   -Wformat-pedantic \
			   -Wfour-char-constants \
			   -Wheader-hygiene \
			   -Widiomatic-parentheses \
			   -Winfinite-recursion \
			   -Wkeyword-macro \
			   -Wmain \
			   -Wmissing-braces \
			   -Wmissing-field-initializers \
			   -Wmissing-include-dirs \
			   -Wmissing-prototypes \
			   -Wmissing-noreturn \
			   -Wnon-virtual-dtor \
			   -Wnull-pointer-arithmetic \
			   -Wold-style-cast \
			   -Wover-aligned \
			   -Woverlength-strings \
			   -Woverloaded-virtual \
			   -Woverriding-method-mismatch \
			   -Wpacked \
			   -Wpessimizing-move \
			   -Wpointer-arith \
			   -Wrange-loop-analysis \
			   -Wredundant-move \
			   -Wredundant-parens \
			   -Wreorder \
			   -Wretained-language-linkage \
			   -Wself-assign \
			   -Wself-move \
			   -Wsemicolon-before-method-body \
			   -Wshadow-all \
			   -Wshift-sign-overflow \
			   -Wshorten-64-to-32 \
			   -Wsign-compare \
			   -Wsign-conversion \
			   -Wsometimes-uninitialized \
			   -Wstatic-in-inline \
			   -Wstrict-prototypes \
			   -Wstring-conversion \
			   -Wsuper-class-method-mismatch \
			   -Wswitch-enum \
			   -Wtautological-overlap-compare \
			   -Wthread-safety \
			   -Wundef \
			   -Wundefined-func-template \
			   -Wundefined-internal-type \
			   -Wundefined-reinterpret-cast \
			   -Wuninitialized \
			   -Wunneeded-internal-declaration \
			   -Wunneeded-member-function \
			   -Wunreachable-code-aggressive \
			   -Wunused-const-variable \
			   -Wunused-function \
			   -Wunused-label \
			   -Wunused-lambda-capture \
			   -Wunused-local-typedef \
			   -Wunused-macros \
			   -Wunused-member-function \
			   -Wunused-parameter \
			   -Wunused-private-field \
			   -Wunused-variable \
			   -Wused-but-marked-unused \
			   -Wuser-defined-literals \
			   -Wvector-conversion \
			   -Wvla \
			   -Wweak-template-vtables \
			   -Wweak-vtables \
			   -Wzero-as-null-pointer-constant \
			   -Wzero-length-array \
			   -Wc++2a-compat \
			   -Wno-weak-vtables \
			   $(CLANG_SANITISER_FLAGS)
GCC_CXXFLAGS=\
			 $(COMMON_CXXFLAGS) \
			 -fdiagnostics-color=always \
			 -Wctor-dtor-privacy \
			 -Wnon-virtual-dtor \
			 -Wreorder \
			 -Woverloaded-virtual \
			 -Wundef \
			 -Wstrict-overflow=2 \
			 -Winit-self \
			 -Wzero-as-null-pointer-constant \
			 -Wuseless-cast \
			 -Wconversion \
			 -Wshadow \
			 -Wswitch-default \
			 -Wswitch-enum \
			 -Wredundant-decls \
			 -Wlogical-op \
			 -Wmissing-include-dirs \
			 -Wmissing-declarations \
			 -Wcast-align \
			 -Wcast-qual \
			 -Wold-style-cast \
			 -Walloc-zero \
			 -Wdouble-promotion \
			 -Wunused-const-variable=2 \
			 -Wduplicated-branches \
			 -Wduplicated-cond \
			 -Wsign-conversion \
			 -Wrestrict \
			 -Wstack-protector \
			 $(GCC_SANITISER_FLAGS)


OPTIMISATION_LEVEL=0
OPTIMISATION_FLAG=-O$(OPTIMISATION_LEVEL)


CXXFLAGS=\
		 -std=$(CXX_STANDARD) \
		 -DSTREAM_BUFFER_COMMIT="\"$(shell ./scripts/get_head_commit.sh)\"" \
		 -DSTREAM_BUFFER_FINGERPRINT="\"$(shell ./scripts/get_code_fingerprint.sh)\"" \
		 -g \
		 $(COMPILER_FLAGS) \
		 $(OPTIMISATION_FLAG) \
		 $(INCLUDE_FLAGS) \
		 -lpthread


PREFIX=/usr/local
BIN_PATH=$(PREFIX)/bin
MAN_PATH=$(PREFIX)/man

.SUFFIXES: .cpp .h .o

.PHONY: \
	all \
	clean \
	install \
	uninstall \
	format \
	watch

all: \
	build/stream-buffer \
	build/stream-buffer-ctl

build/%.o: src/%.cpp
	@echo "$(CXX) -> $@"
	@$(CXX) $(CXXFLAGS) -c -o $@ $<

build/stream-buffer: \
	build/main.o \
	build/buffer.o \
	build/common.o
	@echo "$@"
	@$(CXX) $(CXXFLAGS) -o $@ $^

build/stream-buffer-ctl: \
	build/ctl.o \
	build/common.o
	@echo "$@"
	@$(CXX) $(CXXFLAGS) -o $@ $^

clean:
	rm -rf build
	@mkdir build
	@touch build/.gitkeep

install:
	@cp -v ./build/stream-buffer $(BIN_PATH)/
	@cp -v ./build/stream-buffer-ctl $(BIN_PATH)/
	@mkdir -p $(MAN_PATH)/man1
	@cp -v ./stream-buffer.1 $(MAN_PATH)/man1/

uninstall:
	rm -f $(BIN_PATH)/stream-buffer
	rm -f $(BIN_PATH)/stream-buffer-ctl
	rm -f $(MAN_PATH)/man1/stream-buffer.1

format:
	find ./src -name '*.cpp' | xargs -n 1 --verbose clang-format -i
	find ./include -name '*.h' | xargs -n 1 --verbose clang-format -i

watch:
	(find . -name '*.h' ; find . -name '*.cpp') | entr -c make -j
