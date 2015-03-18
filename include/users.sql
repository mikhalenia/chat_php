--
-- Table structure for table `chat_users`
--

CREATE TABLE IF NOT EXISTS `chat_users` (
	`login` varchar(128) DEFAULT NULL,
	`passwd_left` bigint(20) unsigned NOT NULL,
	`passwd_right` bigint(20) unsigned NOT NULL,
	PRIMARY KEY (`login`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 ;
