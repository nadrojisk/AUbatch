# AUbatch

## Purpose

This repository is for Project 3 for the Advanced Operating Systems class offered at Auburn Univeristy (Spring 2020) by Dr. Xiao Qin.

## Summary

In this project we were required to design and implement an interactive batch scheduler.
I was tasked with implementing the following schedulers: `shortests job first`, `first come first server`, and `priority based`.

## Structure

For this project students were only provided some sample code and specification documents.
All the documents provided by the professor were placed in `assets/res`. 
Inside `assets/doc` and `assets/img` are files related to the final report.
`assets/script` contain script files that were required by Dr. Qin for the final submission.

`src` contains all the source code that I implemented.

`commandline.c/h` provides the command line interface which is interacted with by the user to submit jobs.

`modules.c/h` provides the implementation for the scheduling and dispatcher module. 
This file is used by commandline to submit jobs.

`aubatch.c` is the main driver for this project and the only file that contains a `main` function.
This will set up all the necessary global variables, threads and mutexes needed by the project. 

`microbatch.c` is a test program that is intended to be called by aubatch.
All the program does is sleep for `argv[2]` time.
This is important as when you load a program into AUbatch it expects the amount of time the program should run for.
With `microbatch` you can get semi-accurate metrics.

## Compilation

To compile this project run `make` while in the main directory. 
Most \*NIX systems that have the gcc tooltain should be able to compile this without any issue.
After the project is compiled you can run `.src/aubatch` and you will be dropped into a pseduo terminal.
