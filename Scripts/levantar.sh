#! /bin/bash

cd /home/utnso/workspace

#Clon del repo

git clone https://github.com/sisoputnfrba/tp-2019-2c-No-C-Nada.git

#Creo variables de entorno libMUSE - biblioNOC - biblioSuse

#sudo nano /home/utnso/.profile 

#export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:"/home/utnso/workspace/tp-2019-2c-No-C-Nada/biblioNOC/Debug":"/home/utnso/workspace/tp-2019-2c-No-C-Nada/libSuse/Debug":"/home/utnso/workspace/tp-2019-2c-No-C-Nada/libMUSE/Debug"

#Levanto  .so

git clone https://github.com/sisoputnfrba/tp-2019-2c-No-C-Nada.git

cd biblioNOC/Debug 
make clean
make all

cd ..
cd ..
cd libMUSE/Debug
make clean
make all

cd ..
cd ..
cd biblioSuse/Debug
make clean
make all

cd .


