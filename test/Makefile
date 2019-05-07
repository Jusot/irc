BIN = ../build/npcp

.PHONY: all clean tests grade

all: $(BIN)
	
$(BIN): ../Makefile
	cd .. ; make	
	
clean:
	cd .. ; make clean

tests: $(BIN)
	python3 -m pytest tests/ --chirc-exe=$(BIN) --json=tests/report.json --html=tests/report.html $(TEST_ARGS)

conn: $(BIN)
	python3 -m pytest tests/chirc/tests/test_connection.py --chirc-exe=$(BIN)

lusers_motd: $(BIN)
	python3 -m pytest tests/chirc/tests/test_lusers_motd.py --chirc-exe=$(BIN)

whois: $(BIN)
	python3 -m pytest tests/chirc/tests/test_whois.py --chirc-exe=$(BIN)

robustness: $(BIN)
	python3 -m pytest tests/chirc/tests/test_robustness.py --chirc-exe=$(BIN)

grade: 
	@test -s tests/report.json || { echo; echo "Test report file (tests/report.json) does not exist."; echo "Cannot generate grade without it. Make sure you run the tests first."; echo; exit 1; }
	python3 tests/grade.py --tests-file ./alltests --report-file tests/report.json
