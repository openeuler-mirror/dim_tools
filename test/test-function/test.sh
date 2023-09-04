#!/bin/bash
# Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.

if [[ $1 = "gcov" ]]; then
	TEST_GCOV=1
else
	TEST_GCOV=0
fi

DIM_GEN_BASELINE=../../src/dim_gen_baseline
TEST_EXEC=test_exec/test_exec
TEST_SO=test_so/test_so.so
TEST_MOD=test_mod/test_mod.ko
TEST_MOD_NAME=test_mod
TEST_KERNEL=test_kernel/test_kernel
TEST_KERNEL_NAME=$(uname -r)
TEST_DIR=test
TEST_LNK_EXEC=test_lnk_exec
TEST_LNK_DIR=test_lnk_dir
TEST_4K_FILE_DIR=test_4k_file
TEST_4K_FILE_GEN=gen.sh

TEST_CASE_NUM=0
TEST_CASE_SUCCESS_NUM=0
TEST_CASE_FAIL_NUM=0
TEST_SUB_CASE_NUM=0
TEST_SUB_CASE_SUCCESS_NUM=0
TEST_SUB_CASE_FAIL_NUM=0
TEST_FAIL=0
TEST_CASE_FAIL=0
TEST_SUB_CASE_FAIL=0

test_init() {
	make clean -C ../src &> /dev/null

	if [ $TEST_GCOV -eq 1 ]; then
		make -C ../src DIM_TOOLS_GCOV=1 &> /dev/null
	else
		make -C ../src &> /dev/null
	fi

	for dir in test_exec test_mod test_so; do
		make -C $dir &> /dev/null || { echo "Fail to build test code!"; exit 1; }
	done

	mkdir -p $TEST_DIR
	ln -s $TEST_EXEC $TEST_LNK_EXEC
	ln -s . $TEST_LNK_DIR
	rm -f test.hash
	rm -f print.log
}

test_exit() {
	if [ $TEST_FAIL -eq 1 ]; then
		echo "TEST RESULT: FAIL!"
	else
		echo "TEST RESULT: SUCCESS!"
	fi

	echo "TEST CASES: $TEST_CASE_NUM"
	echo "SUCCESS CASES: $TEST_CASE_SUCCESS_NUM"
	echo "FAIL CASES: $TEST_CASE_FAIL_NUM"
	echo "TEST SUB_CASES: $TEST_SUB_CASE_NUM"
	echo "SUCCESS SUB_CASES: $TEST_SUB_CASE_SUCCESS_NUM"
	echo "FAIL SUB_CASES: $TEST_SUB_CASE_FAIL_NUM"

	for dir in test_exec test_mod test_so; do
		make clean -C $dir &> /dev/null
	done

	rm -rf $TEST_DIR $TEST_LNK_EXEC $TEST_LNK_DIR test.hash
	rm -f */*.hash
	rm -f $TEST_4K_FILE_DIR/test*
	return $TEST_FAIL
}

test_gcov() {
	if [ $TEST_GCOV -eq 1 ]; then
		lcov -d ../../src -o test.info -b ../../src -c
		genhtml -o result_gcov test.info
	fi
}

test_sub_case_start() {
	#echo "---- sub-case ${FUNCNAME[1]} start! ----"
	TEST_SUB_CASE_NUM=$(($TEST_SUB_CASE_NUM+1))
}

test_sub_case_finish() {
	if [ $TEST_SUB_CASE_FAIL -eq 0 ]; then
		echo "---- sub-case ${FUNCNAME[1]} success! ----"
		TEST_SUB_CASE_SUCCESS_NUM=$(($TEST_SUB_CASE_SUCCESS_NUM+1))
	else
		echo "---- sub-case ${FUNCNAME[1]} fail! ----"
		TEST_SUB_CASE_FAIL_NUM=$(($TEST_SUB_CASE_FAIL_NUM+1))
		TEST_SUB_CASE_FAIL=0
		TEST_CASE_FAIL=1
	fi
	rm -f test.hash print.log
}

test_case_start() {
	echo "--- case ${FUNCNAME[1]} start! ---"
	TEST_CASE_NUM=$(($TEST_CASE_NUM+1))
}

test_case_finish() {
	if [ $TEST_CASE_FAIL -eq 0 ]; then
		echo "--- case ${FUNCNAME[1]} success! ---"
		TEST_CASE_SUCCESS_NUM=$(($TEST_CASE_SUCCESS_NUM+1))
	else
		echo "--- case ${FUNCNAME[1]} fail! ---"
		TEST_CASE_FAIL_NUM=$(($TEST_CASE_FAIL_NUM+1))
		TEST_FAIL=1
		TEST_CASE_FAIL=0
	fi
}

assert_num_not_zero() {
	if [ $1 -eq 0 ]; then
		TEST_SUB_CASE_FAIL=1
	fi
}

assert_num_zero() {
	if [ $1 -ne 0 ]; then
		TEST_SUB_CASE_FAIL=1
	fi
}

assert_num_equal() {
	if [ $1 -ne $2 ]; then
		TEST_SUB_CASE_FAIL=1
	fi
}

assert_file_exist() {
	if [ ! -f $1 ]; then
		TEST_SUB_CASE_FAIL=1
	fi
}

assert_file_not_exist() {
	if [ -f $1 ]; then
		TEST_SUB_CASE_FAIL=1
	fi
}

assert_string_not_empty() {
	if [ -z $1 ]; then
		TEST_SUB_CASE_FAIL=1
	fi
}

assert_print_contain() {
	grep "$1" print.log > /dev/null
	if [ $? -ne 0 ]; then
		TEST_SUB_CASE_FAIL=1
	fi
}

## TEST SUB-CASES ##
test_help() {
	test_sub_case_start
	$DIM_GEN_BASELINE -h &> print.log
	assert_num_zero $?
	test_sub_case_finish
}

test_calculate_hash_exec() {
	test_sub_case_start
	sh get_code_hash.sh $TEST_EXEC
	assert_num_zero $?
	$DIM_GEN_BASELINE -o test.hash $TEST_EXEC
	assert_num_zero $?
	assert_file_exist test.hash
	count=$(grep -Ec "dim USER sha256:$(cat $TEST_EXEC.code.hash) $(realpath $TEST_EXEC)" test.hash)
	assert_num_equal $count 1
	test_sub_case_finish
}

test_calculate_hash_so() {
	test_sub_case_start
	sh get_code_hash.sh $TEST_SO
	assert_num_zero $?
	$DIM_GEN_BASELINE -o test.hash $TEST_SO
	assert_num_zero $?
	assert_file_exist test.hash
	count=$(grep -Ec "dim USER sha256:$(cat $TEST_SO.code.hash) $(realpath $TEST_SO)" test.hash)
	assert_num_equal $count 1
	test_sub_case_finish
}

test_calculate_hash_mod() {
	test_sub_case_start
	sha256sum $TEST_MOD | awk '{print $1}' > $TEST_MOD.hash
	assert_num_zero $?
	$DIM_GEN_BASELINE -o test.hash $TEST_MOD
	assert_num_zero $?
	assert_file_exist test.hash
	count=$(grep -Ec "dim KERNEL sha256:$(cat $TEST_MOD.hash) $(uname -r)/$TEST_MOD_NAME" test.hash)
	assert_num_equal $count 1
	test_sub_case_finish
}

test_calculate_hash_text() {
	test_sub_case_start
	$DIM_GEN_BASELINE -o test.hash $TEST_KERNEL &> print.log
	assert_num_not_zero $?
	assert_file_not_exist test.hash
	assert_print_contain "No valid baseline data"
	test_sub_case_finish
}

test_calculate_hash_dir() {
	test_sub_case_start
	$DIM_GEN_BASELINE -o test.hash $TEST_DIR &> print.log
	assert_num_not_zero $?
	assert_file_not_exist test.hash
	assert_print_contain "No valid baseline data"
	test_sub_case_finish
}

test_calculate_hash_lnk() {
	test_sub_case_start
	$DIM_GEN_BASELINE -o test.hash $TEST_LNK_EXEC &> print.log
	assert_num_not_zero $?
	assert_file_not_exist test.hash
	assert_print_contain "No valid baseline data"
	test_sub_case_finish
}

test_calculate_hash_not_exist() {
	test_sub_case_start
	$DIM_GEN_BASELINE -o test.hash ./invalid &> print.log
	assert_num_not_zero $?
	assert_file_not_exist test.hash
	assert_print_contain "No valid baseline data"
	test_sub_case_finish
}

test_calculate_hash_kernel() {
	test_sub_case_start
	sha256sum $TEST_KERNEL | awk '{print $1}' > $TEST_KERNEL.hash
	assert_num_zero $?
	$DIM_GEN_BASELINE -k $TEST_KERNEL_NAME -o test.hash $TEST_KERNEL
	assert_num_zero $?
	count=$(grep -Ec "dim KERNEL sha256:$(cat $TEST_KERNEL.hash) $TEST_KERNEL_NAME" test.hash)
	assert_num_equal $count 1
	test_sub_case_finish
}

test_calculate_hash_kernel_long_name() {
	test_sub_case_start
	sha256sum $TEST_KERNEL | awk '{print $1}' > $TEST_KERNEL.hash
	assert_num_zero $?
	name=$(printf 'a%.0s' {1..255})
	$DIM_GEN_BASELINE -k $name -o test.hash $TEST_KERNEL
	assert_num_zero $?
	assert_file_exist test.hash
	count=$(grep -Ec "dim KERNEL sha256:$(cat $TEST_KERNEL.hash) $name" test.hash)
	assert_num_equal $count 1
	test_sub_case_finish
}

test_calculate_hash_kernel_too_long_name() {
	test_sub_case_start
	name=$(printf 'a%.0s' {1..256})
	$DIM_GEN_BASELINE -k $name -o test.hash $TEST_KERNEL &> print.log
	assert_num_not_zero $?
	assert_file_not_exist test.hash
	assert_print_contain "Kernel release name too long"
	test_sub_case_finish
}

test_hash_algo() {
	test_sub_case_start
	$DIM_GEN_BASELINE -a sha256 $TEST_EXEC -o test.hash
	assert_num_zero $?
	assert_file_exist test.hash
	test_sub_case_finish
}

test_hash_algo_invalid() {
	test_sub_case_start
	$DIM_GEN_BASELINE -a md5 $TEST_EXEC -o test.hash &> print.log
	assert_num_not_zero $?
	assert_file_not_exist test.hash
	assert_print_contain "Unsupported hash algorithm"
	test_sub_case_finish
}

test_relative_root() {
	test_sub_case_start
	$DIM_GEN_BASELINE -R . $TEST_EXEC -o test.hash
	assert_num_zero $?
	assert_file_exist test.hash
	grep " /$TEST_EXEC" test.hash &> /dev/null
	assert_num_zero $?
	test_sub_case_finish
}

test_relative_root_not_parent() {
	test_sub_case_start
	$DIM_GEN_BASELINE -R /proc $TEST_EXEC -o test.hash &> print.log
	assert_num_not_zero $?
	assert_file_not_exist test.hash
	assert_print_contain "is not under the relative root"
	test_sub_case_finish
}

test_relative_root_not_exist() {
	test_sub_case_start
	$DIM_GEN_BASELINE -R ./invalid $TEST_EXEC -o test.hash &> print.log
	assert_num_not_zero $?
	assert_file_not_exist test.hash
	assert_print_contain "No such file or directory"
	test_sub_case_finish
}

test_relative_root_prefix() {
	test_sub_case_start
	$DIM_GEN_BASELINE -R $TEST_DIR $TEST_EXEC -o test.hash &> print.log
	assert_num_not_zero $?
	assert_file_not_exist test.hash
	assert_print_contain "is not under the relative root"
	test_sub_case_finish
}

test_relative_root_lnk() {
	test_sub_case_start
	$DIM_GEN_BASELINE -R $TEST_LNK_DIR $TEST_EXEC -o test.hash
	assert_num_zero $?
	assert_file_exist test.hash
	grep " /$TEST_EXEC" test.hash &> /dev/null
	assert_num_zero $?
	test_sub_case_finish
}

test_relative_root_text() {
	test_sub_case_start
	$DIM_GEN_BASELINE -R $TEST_KERNEL $TEST_EXEC -o test.hash &> print.log
	assert_num_not_zero $?
	assert_print_contain "is not directory"
	test_sub_case_finish
}

test_output() {
	test_sub_case_start
	$DIM_GEN_BASELINE $TEST_EXEC -o test.hash
	assert_num_zero $?
	assert_file_exist test.hash
	test_sub_case_finish
}

test_output_directory() {
	test_sub_case_start
	$DIM_GEN_BASELINE $TEST_EXEC -o $TEST_DIR &> print.log
	assert_num_not_zero $?
	assert_print_contain "Is a directory"
	test_sub_case_finish
}

test_calculate_hash_recursive_exec() {
	test_sub_case_start
	$DIM_GEN_BASELINE -r $TEST_EXEC -o test.hash
	assert_num_zero $?
	assert_file_exist test.hash
	test_sub_case_finish
}

test_calculate_hash_recursive_dir() {
	test_sub_case_start
	$DIM_GEN_BASELINE -r . -o test.hash
	assert_num_zero $?
	assert_file_exist test.hash
	for item in $TEST_EXEC $TEST_SO $TEST_MOD_NAME; do
		assert_string_not_empty $(grep $item test.hash)
	done
	test_sub_case_finish
}

test_calculate_hash_recursive_dir_empty() {
	test_sub_case_start
	$DIM_GEN_BASELINE -r $TEST_DIR -o test.hash &> print.log
	assert_num_not_zero $?
	assert_file_not_exist test.hash
	assert_print_contain "No valid baseline data"
	test_sub_case_finish
}

test_calculate_hash_recursive_lnk_dir() {
	test_sub_case_start
	# TEST_LNK_DIR is lnk to .
	$DIM_GEN_BASELINE -r $TEST_LNK_DIR -o test.hash &> print.log
	assert_num_not_zero $?
	assert_file_not_exist test.hash
	assert_print_contain "No valid baseline data"
	test_sub_case_finish
}

test_calculate_hash_recursive_not_exist() {
	test_sub_case_start
	$DIM_GEN_BASELINE -r ./invalid -o test.hash &> print.log
	assert_num_not_zero $?
	assert_file_not_exist test.hash
	assert_print_contain "No such file or directory"
	test_sub_case_finish
}

test_test_calculate_hash_exec_verbose() {
	test_sub_case_start
	$DIM_GEN_BASELINE -v $TEST_EXEC -o test.hash &> print.log
	assert_num_zero $?
	assert_file_exist test.hash
	assert_print_contain "Finish calculating baseline"
	test_sub_case_finish
}

test_calculate_hash_text_verbose() {
	test_sub_case_start
	$DIM_GEN_BASELINE -v $TEST_KERNEL -o test.hash &> print.log
	assert_num_not_zero $?
	assert_file_not_exist test.hash
	assert_print_contain "not an elf file"
	test_sub_case_finish
}

## TEST CASES  ##
test_dim_tools_cmd_algo() {
	test_case_start
	test_hash_algo
	test_hash_algo_invalid
	test_case_finish
}

test_dim_tools_cmd_help() {
	test_case_start
	test_help
	test_case_finish
}

test_dim_tools_cmd_kernel() {
	test_calculate_hash_kernel
	test_case_start
	test_calculate_hash_kernel_long_name
	test_calculate_hash_kernel_too_long_name
	test_case_finish
}

test_dim_tools_cmd_output() {
	test_case_start
	test_output
	test_output_directory
	test_case_finish
}

test_dim_tools_cmd_relative_path() {
	test_case_start
	test_relative_root
	test_relative_root_not_parent
	test_relative_root_not_exist
	test_relative_root_prefix
	test_relative_root_lnk
	test_relative_root_text
	test_case_finish
}

test_dim_tools_cmd_recursive() {
	test_case_start
	test_calculate_hash_recursive_exec
	test_calculate_hash_recursive_dir
	test_calculate_hash_recursive_dir_empty
	test_calculate_hash_recursive_lnk_dir
	test_calculate_hash_recursive_not_exist
	test_case_finish
}

test_dim_tools_cmd_verbose() {
	test_case_start
	test_test_calculate_hash_exec_verbose
	test_calculate_hash_text_verbose
	test_case_finish
}

test_dim_tools_cmd_path() {
	test_case_start
	test_calculate_hash_exec
	test_calculate_hash_so
	test_calculate_hash_mod
	test_calculate_hash_text
	test_calculate_hash_dir
	test_calculate_hash_lnk
	test_calculate_hash_not_exist
	test_case_finish
}

## TEST MAIN  ##
test_init
test_dim_tools_cmd_algo
test_dim_tools_cmd_help
test_dim_tools_cmd_kernel
test_dim_tools_cmd_output
test_dim_tools_cmd_relative_path
test_dim_tools_cmd_recursive
test_dim_tools_cmd_verbose
test_dim_tools_cmd_path
test_gcov
test_exit

