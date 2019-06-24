BISON_INPUT=json_parser.y
FLEX_INPUT=json_parser.l

TEST_FILE=testcase.json

BUILD_DIR=./build

COMPILER=g++
WARNINGS= 


_IN_BUILD = cd $(BUILD_DIR);

all: 
	mkdir -p $(BUILD_DIR);
	cp $(BISON_INPUT) $(BUILD_DIR)/$(BISON_INPUT)
	cp $(FLEX_INPUT) $(BUILD_DIR)/$(FLEX_INPUT)
	cp *.h $(BUILD_DIR)/
	cp json_classes.cpp $(BUILD_DIR)/
	$(_IN_BUILD) bison -y -d $(BISON_INPUT)
	$(_IN_BUILD) flex $(FLEX_INPUT)
	$(_IN_BUILD) $(COMPILER) -c json_classes.cpp $(WARNINGS)
	$(_IN_BUILD) $(COMPILER) -c y.tab.c lex.yy.c $(WARNINGS)
	$(_IN_BUILD) $(COMPILER) y.tab.o lex.yy.o json_classes.o -o parser $(WARNINGS)
	cp build/parser ./parser.out

test: all
	./parser.out testcase.json

clean:
	rm $(BUILD_DIR) -rf