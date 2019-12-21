#! /bin/bash -e
#Flag e para que en caso de error cierre el script


MYFIRSTPATH="/home/utnso/workspace"
MYPATH ="/home/utnso/workspace/tp-2019-2c-No-C-Nada"



#Creo carpeta si no existe
mkdir -p workspace

cd "$MYFIRSTPATH"

#Clon del repo

git clone https://github.com/sisoputnfrba/tp-2019-2c-No-C-Nada.git 

#Creo variables de entorno libMUSE - biblioNOC - biblioSuse

sudo nano /home/utnso/.profile 

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:"/home/utnso/workspace/tp-2019-2c-No-C-Nada/biblioNOC/Debug":"/home/utnso/workspace/tp-2019-2c-No-C-Nada/biblioSuse/Debug":"/home/utnso/workspace/tp-2019-2c-No-C-Nada/libMUSE/Debug"

echo LD_LIBRARY_PATH

#Levanto  .so

git clone https://github.com/sisoputnfrba/tp-2019-2c-No-C-Nada.git

cd "$MYPATH/biblioNOC/Debug" 
make clean
make all

cd "$MYPATH/libMUSE/Debug"
make clean
make all

cd "$MYPATH/biblioSuse/Debug"
make clean
make all
