NAME		= Oneshot

LANGUAGE	?= en

CC		= i686-w64-mingw32.shared-gcc
LD		= i686-w64-mingw32.shared-ld
CC_LINUX	= gcc
WINDRES		= i686-w64-mingw32.shared-windres
ICOTOOL		= icotool
PNG2ICNS	= png2icns
ENCRYPT		= encrypt

SRC_DIR		= .
OBJ_DIR		= obj

DLL_CFLAGS	= -Wall -O3
DLL_TARGET	= $(SRC_DIR)/exe/embed/hook.dll
DLL_LDFLAGS	= -lshlwapi -lgdi32 -lsecur32 -lole32 -lstrmiids -lshell32 -lsndfile -shared -ldxguid -lddraw -O3
EXE_CFLAGS	= -Wall -O3
EXE_TARGET	= RPG_RT.exe
EXE_LDFLAGS	= -mwindows -lshlwapi -lshell32 -O3

DLL_RC		= $(SRC_DIR)/dll/resource.$(LANGUAGE).rc
EXE_RC		= $(SRC_DIR)/exe/resource.$(LANGUAGE).rc

DLL_SRC_FILES	= $(wildcard $(SRC_DIR)/dll/*.c)
EXE_SRC_FILES	= $(wildcard $(SRC_DIR)/exe/*.c)

DLL_OBJ_FILES=$(DLL_SRC_FILES:$(SRC_DIR)/dll/%.c=$(OBJ_DIR)/dll/%.o) $(DLL_RC:$(SRC_DIR)/dll/%.rc=$(OBJ_DIR)/dll/%.o)
EXE_OBJ_FILES=$(EXE_SRC_FILES:$(SRC_DIR)/exe/%.c=$(OBJ_DIR)/exe/%.o) $(EXE_RC:$(SRC_DIR)/exe/%.rc=$(OBJ_DIR)/exe/%.o)

PAPER_TARGET_32	= $(SRC_DIR)/exe/embed/paper32
PAPER_TARGET_64	= $(SRC_DIR)/exe/embed/paper64
PAPER_SRC_FILE	= $(SRC_DIR)/paper/main.c
PAPER_CFLAGS	= -Wall -O3 -lX11

ICO_TARGET	= $(SRC_DIR)/icon.ico
ICNS_TARGET	= $(SRC_DIR)/icon.icns

ICON_SRC_DIR	= $(SRC_DIR)/icon.iconset
ICON_SRC_FILES	= $(wildcard $(ICON_SRC_DIR)/*.png)

EDITOR_NAME		= RPG2003Launcher
EDITOR_DIR		= bin
EDITOR_DLL_CFLAGS	= -Wall -O3
EDITOR_DLL_TARGET	= $(EDITOR_DIR)/$(EDITOR_NAME).dll
EDITOR_DLL_LDFLAGS	= -lshlwapi -lsndfile -shared -O3
EDITOR_EXE_CFLAGS	= -Wall -O3
EDITOR_EXE_TARGET	= $(EDITOR_DIR)/$(EDITOR_NAME).exe
EDITOR_EXE_LDFLAGS	= -lshlwapi -mwindows -O3

EDITOR_SRC_DIR		= $(SRC_DIR)/editor
EDITOR_OBJ_DIR		= $(OBJ_DIR)/editor

EDITOR_DLL_SRC_FILES	= $(wildcard $(EDITOR_SRC_DIR)/dll/*.c)
EDITOR_EXE_SRC_FILES	= $(wildcard $(EDITOR_SRC_DIR)/exe/*.c)

EDITOR_DLL_OBJ_FILES=$(EDITOR_DLL_SRC_FILES:$(EDITOR_SRC_DIR)/dll/%.c=$(EDITOR_OBJ_DIR)/dll/%.o)
EDITOR_EXE_OBJ_FILES=$(EDITOR_EXE_SRC_FILES:$(EDITOR_SRC_DIR)/exe/%.c=$(EDITOR_OBJ_DIR)/exe/%.o)

ENCRYPT_TARGET		= $(SRC_DIR)/exe/embed/RPG_RT.exe.xor

ifeq ($(LANGUAGE),en)
	ENCRYPT_EXE	= english.exe
else ifeq ($(LANGUAGE),ko)
	ENCRYPT_EXE	= korean.exe
	EXE_CFLAGS	+= -DL_KOREAN
	DLL_CFLAGS	+= -DL_KOREAN
else ifeq ($(LANGUAGE),ru)
	ENCRYPT_EXE = english.exe
	EXE_CFLAGS  += -DL_RUSSIAN
	DLL_CFLAGS  += -DL_RUSSIAN
else ifeq ($(LANGUAGE),es)
	ENCRYPT_EXE = english.exe
	EXE_CFLAGS  += -DL_SPANISH
	DLL_CFLAGS  += -DL_SPANISH
endif

all: $(ENCRYPT_TARGET) $(ICO_TARGET) $(ICNS_TARGET) $(EXE_TARGET) $(DLL_TARGET)

editor: $(EDITOR_DLL_TARGET) $(EDITOR_EXE_TARGET)

$(ENCRYPT_TARGET): $(SRC_DIR)/exe/embed/$(ENCRYPT_EXE)
	$(ENCRYPT) $< $@

$(ICO_TARGET): $(ICON_SRC_FILES)
	$(ICOTOOL) -c $(ICON_SRC_FILES) -o $(ICO_TARGET)

$(ICNS_TARGET): $(ICON_SRC_FILES)
	$(PNG2ICNS) $(ICNS_TARGET) $(ICON_SRC_FILES)

$(PAPER_TARGET_32): $(PAPER_SRC_FILE)
	$(CC_LINUX) $< -o $@ $(PAPER_CFLAGS) -m32
	#upx -q -q $@

$(PAPER_TARGET_64): $(PAPER_SRC_FILE)
	$(CC_LINUX) $< -o $@ $(PAPER_CFLAGS) -m64
	#upx -q -q $@

$(EXE_TARGET): $(EXE_OBJ_FILES)
	$(CC) $(EXE_OBJ_FILES) -o $(EXE_TARGET) $(EXE_LDFLAGS)

$(DLL_TARGET): $(DLL_OBJ_FILES)
	$(CC) $(DLL_OBJ_FILES) -o $(DLL_TARGET) $(DLL_LDFLAGS)
	$(ENCRYPT) $(DLL_TARGET) $(DLL_TARGET).xor

$(OBJ_DIR)/exe/%.o: $(SRC_DIR)/exe/%.c | $(OBJ_DIR)
	$(CC) -c $< -o $@ $(EXE_CFLAGS)

$(OBJ_DIR)/dll/%.o: $(SRC_DIR)/dll/%.c | $(OBJ_DIR)
	$(CC) -c $< -o $@ $(DLL_CFLAGS)

$(OBJ_DIR)/exe/%.o: $(SRC_DIR)/exe/%.rc $(ICO_TARGET) $(PAPER_TARGET_32) $(PAPER_TARGET_64) $(DLL_TARGET) $(SRC_DIR)/exe/embed
	$(WINDRES) $< $@

$(OBJ_DIR)/dll/%.o: $(SRC_DIR)/dll/%.rc $(ICO_TARGET) $(SRC_DIR)/dll/embed
	$(WINDRES) $< $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)/exe $(OBJ_DIR)/dll

$(EDITOR_EXE_TARGET): $(EDITOR_EXE_OBJ_FILES) | $(EDITOR_DIR)
	$(CC) $(EDITOR_EXE_OBJ_FILES) -o $(EDITOR_EXE_TARGET) $(EDITOR_EXE_LDFLAGS)

$(EDITOR_DLL_TARGET): $(EDITOR_DLL_OBJ_FILES) | $(EDITOR_DIR)
	$(CC) $(EDITOR_DLL_OBJ_FILES) -o $(EDITOR_DLL_TARGET) $(EDITOR_DLL_LDFLAGS)

$(EDITOR_OBJ_DIR)/exe/%.o: $(EDITOR_SRC_DIR)/exe/%.c | $(EDITOR_OBJ_DIR)
	$(CC) -c $< -o $@ $(EDITOR_EXE_CFLAGS)

$(EDITOR_OBJ_DIR)/dll/%.o: $(EDITOR_SRC_DIR)/dll/%.c | $(EDITOR_OBJ_DIR)
	$(CC) -c $< -o $@ $(EDITOR_DLL_CFLAGS)

$(EDITOR_DIR):
	mkdir -p $(EDITOR_DIR)

$(EDITOR_OBJ_DIR):
	mkdir -p $(EDITOR_OBJ_DIR)/exe $(EDITOR_OBJ_DIR)/dll

clean:
	rm -rf $(OBJ_DIR)
	rm -f $(ICO_TARGET) $(ICNS_TARGET) $(PAPER_TARGET_32) $(PAPER_TARGET_64) $(EXE_TARGET) $(DLL_TARGET)
	rm -f $(SRC_DIR)/exe/embed/RPG_RT.exe.xor

editor-clean:
	rm -rf $(EDITOR_OBJ_DIR) $(EDITOR_DIR)

.PHONY: all editor clean editor-clean dist-win
