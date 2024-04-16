simple_shell: 2024_lab2_shellwithTODO.c
	gcc -o simple_shell 2024_lab2_shellwithTODO.c

# TODO: 参照 simple_shell，增加编译 testsh 的规则
testsh: testsh.c
	gcc -o testsh testsh.c

#TODO:添加自动化测试的lable
#	.PHONY  表示伪标签
.PHONY : clean test

clean:
	rm -f testsh simple_shell

#TODO: 参照 clean ，增加自动化测试的规则，注意自动化测试的规则是可以有依赖的

test: testsh simple_shell
	./testsh ./simple_shell

#目标：依赖项
#	生成命令