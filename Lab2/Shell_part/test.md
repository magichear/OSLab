### 多条命令

    echo a; echo b; echo c

### 两条命令间的管道

    ps aux | wc -l

### 内建命令

    ls
    mkdir test
    cd test
    pwd
    exit

## 重定向符

### >>

    >> b echo hello ; cat b

### <

### 测试流程

    ps aux | wc -l ; grep USTC < testdata > testsh.out ; >> a echo 66666666 ; cat a
    rm a

    ls
    mkdir test
    cd test
    pwd
    exit

    rm -r test

    make test
