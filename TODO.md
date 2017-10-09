* roadmap for wordseg-0.5 [0/8]
- [ ] ag output: make it works
- [ ] implement --ag-median option and run 8 times in the inner loop
- [ ] populate test/test_ag.py
- [ ] test AG_PARALLEL and AG_QUADRUPLE
- [ ] forward dpseg/ag stderr to Python log
- [ ] test ag works from config file
- [ ] add a ChangeLog.md
- [ ] reorganize documentation (less pages for general, more for API)

* to do one of these days
- [ ] add functions in the statistics module
- [ ] refactor segment() in ag and dpseg (easy access to arguments from Python)
- [ ] refactor C++ code in ag (C++11, stdlib, etc...)
- [ ] optimize dpseg and ag
- [ ] make a wordseg.so for dpseg/ag common code
