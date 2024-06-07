simple_shell: 2024_lab2_shellwithTODO.c
	gcc -o simple_shell 2024_lab2_shellwithTODO.c

# 编译 testsh
testsh: testsh.c
	gcc -o testsh testsh.c

# 自动化测试的lable
#	.PHONY  表示伪标签
.PHONY : clean test

clean:
	rm -f testsh simple_shell

# 自动化测试的规则，注意自动化测试的规则是可以有依赖的

test: testsh simple_shell
	./testsh simple_shell

#目标：依赖项
#	生成命令
