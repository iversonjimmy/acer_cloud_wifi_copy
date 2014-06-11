USE testharness;

DROP TABLE IF EXISTS test_run;
CREATE TABLE IF NOT EXISTS test_run (
	id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
        name VARCHAR(128),
	start_time INT,
	end_time INT,
	status VARCHAR(32),
        tc_count INT,
        tc_pass INT,
        tc_fail INT,
        tc_skip INT,
        tc_indeterminate INT,
        tc_remaining INT,
        output_dir VARCHAR(1024)
);

DROP TABLE IF EXISTS test_suite;
CREATE TABLE IF NOT EXISTS test_suite (
	id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
	name VARCHAR(128)
);

DROP TABLE IF EXISTS test_case;
CREATE TABLE IF NOT EXISTS test_case (
	id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
	name VARCHAR(128)
);

DROP TABLE IF EXISTS test_case_instance;
CREATE TABLE IF NOT EXISTS test_case_instance (
	id INT NOT NULL AUTO_INCREMENT PRIMARY KEY,
	test_run_id INT,
	test_suite_id INT,
	test_case_id INT,
	env_vars VARCHAR(1024),
	run_order INT,
	start_time INT,
	end_time INT,
	sub_name VARCHAR(128),
	sub_run_order INT,
	status VARCHAR(32),
	test_text TEXT
);


