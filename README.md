# BudgetEPT

This project is a proof-of-concept (POC) demonstrating a method of using supervisor-mode access prevention (SMAP)
and supervisor-mode execution prevention (SMEP) to create inline hooks that are functionally
similar to extended page table (EPT) hooks. The project also demonstrates a limited
example of how software virtualization could be used in conjunction with this project 
to better hide the presence of such hooks. A more detailed write-up can be found 
[here](https://brew02.github.io/posts/2024/EPT-hooks-on-a-budget.html). 