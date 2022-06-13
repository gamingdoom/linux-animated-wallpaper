# C Template
This repo is a template for C projects. It includes a makefile, a `main.c` file, 
the necessary files to debug C with vscode, a `.gitignore` file, and a shell 
script to change some placeholder names in the template.

# Usage
After you use this template to create a repo by clicking "Use this template", 
run the `setupTemplate.sh` script to change all of the placeholder names to 
their actual names. You can then start writing code in the `main.c` file. All 
files inside the src directory that have the `.c` extention will be compiled 
into the final program when `make` is run. To add libraries (e.g. `-lm`) for gcc 
to compile with, open the makefile and add the argument that you would give to 
gcc to the `LIBS` variable. For example, the line with the `LIBS` variable could 
look like this:
```
LIBS=-lm -lpthread
```
It works the same way for include directories, just add them to the `INCLUDE` 
variable instead of the `LIBS` variable.