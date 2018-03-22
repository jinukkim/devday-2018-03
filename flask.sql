DROP TABLE IF EXISTS `entries`;


CREATE TABLE `entries` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `title` text NOT NULL,
  `body` text NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

LOCK TABLES `entries` WRITE;


UNLOCK TABLES;
