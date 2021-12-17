# Beerbox

## how to build web app

- install mariadb, node and npm and mosquitto
- install npm dependencies by doing npm install in /web/frontend/ and /web/backend/ folders
- change scripts/initialize.sh credentials to your mariadb root user and run it to create correct database and tables
- create user for db that can interact with new database, example in /web/db/user.sql, change password!!
 - then update that user to /web/backend/db.js/
- test front/backend locally with `npm start` if needed.
- build frontend with `npm build` or `npm run deploy` if you are using nginx as webserver, you can also modify the deploy script in package.json to cover your needs
- update mqtt broker information on express/server.js
- run web/backend/server.js with pm2
- host with webserver of your choice