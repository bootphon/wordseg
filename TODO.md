* roadmap for wordseg-0.5 [6/8]
- [X] ag output: make it works
- [X] implement --score-category option
- [X] run 8 times in the inner loop
- [X] test AG_PARALLEL and AG_QUADRUPLE
- [X] be sure about mbr/counter
- [X] populate test/test_ag.py
- [ ] forward dpseg/ag stderr to Python log
* to do one of these days [0/8]
- [ ] add functions in the statistics module
- [ ] refactor segment() in ag and dpseg (easy access to arguments
  from Python)
- [ ] refactor C++ code in ag (C++11, stdlib, etc...)
- [ ] optimize dpseg and ag
- [ ] make a wordseg.so for dpseg/ag common code
- [ ] write a ChangeLog.md
- [ ] refactor the html documentation (less pages for general, more
  pages for API)
- [ ] make wordseg-ag reads parameters from config file
