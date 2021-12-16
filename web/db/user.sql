CREATE USER 'beerbox'@'localhost' IDENTIFIED BY 'beerbox';
GRANT SELECT,INSERT,UPDATE ON beerbox.* TO 'beerbox'@'localhost';