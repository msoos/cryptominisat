<!-- START doctoc generated TOC please keep comment here to allow auto update -->
<!-- DON'T EDIT THIS SECTION, INSTEAD RE-RUN doctoc TO UPDATE -->
**Table of Contents**  *generated with [DocToc](https://github.com/thlorenz/doctoc)*

- [Install MySQL, PHP and Apache2](#install-mysql-php-and-apache2)
- [Set up Apache](#set-up-apache)
- [Build CryptoMiniSat](#build-cryptominisat)
- [Run CryptoMiniSat](#run-cryptominisat)
- [Explore](#explore)

<!-- END doctoc generated TOC please keep comment here to allow auto update -->

Install MySQL, PHP and Apache2
------

```
sudo apt-get install php5 php5-mysqlnd apache2 libapache2-mod-php5 myslq-server
sudo a2enmod php5
./cmsat_mysql_setup.sh [mysql root passowrd if not empty]
```

Do NOT expose Apache to the outside. Do this by changing in `/etc/apache2/ports.conf` the line`Listen 80` to `Listen 127.0.0.1:80`. Similarly, change `Listen 443` to `Listen 127.0.0.1:443`

Set up Apache
------
My `/etc/apache2/sites-enabled/000-default` is:

```
<VirtualHost *:80>
    ServerAdmin webmaster@localhost

    DocumentRoot /home/soos/cryptominisat/web  
    <Directory /home/soos/cryptominisat/web/>
            Options Indexes FollowSymLinks MultiViews
            AllowOverride None
    </Directory>

    ErrorLog ${APACHE_LOG_DIR}/error.log

    LogLevel warn
    CustomLog ${APACHE_LOG_DIR}/access.log combined
</VirtualHost>
```

After suitably fixing yours up (or just using the above), issue `sudo service apache2 restart`.

Build CryptoMiniSat
------

```
rm -rf build
mkdir build && cd build
cmake -DSTATS=ON ..
make
sudo make install
sudo ldconfig
```

Run CryptoMiniSat
------

```
cryptominisat5 --sql 1 --sqlfull 1 file.cnf
```

Explore
-------

Navigate to `localhost` and enjoy!
