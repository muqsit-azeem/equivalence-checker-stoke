#!/bin/bash

stoke debug verify --strategy bounded --target opt1/stlen.s --rewrite opt1/stlen_unroll.s  --def_in "{ %rdi %rsi }" --live_out "{ %rax }" --bound 6