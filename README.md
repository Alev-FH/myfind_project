## Project myfind
Usage:
```
/myfind [-R] [-i] searchpath filename1 [filename2] …[filenameN]
```
The main program accepts the arguments <searchpath> and <filename1 .. N>. Keep in mind that a
variable number of arguments (= variable number of filenames to look for) can be used and that the
options -R and -i can be placed anywhere in the arguments list.

- -R:
    - should switch myfind in recursive mode and find all matching files in and below the
      searchpath folder (else the files should only be searched in the searchpath folder)
- -i
    - case in-sensitive search
- searchpath
    - can be an absolute or a relative path.
- filename
    - only filenames as plain string
    - no support for paths, subpaths, wildcards required.

 Example:
 ```
./myfind ./ test.txt test.doc test
```
Here, myfind searches for the 3 files in the current working directory in parallel.
