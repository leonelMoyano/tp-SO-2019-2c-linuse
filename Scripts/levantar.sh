#! /bin/bash -e
#Flag error -> cierre del script

#Repo tp-2019-2c-No-C-Nada: git clone https://github.com/sisoputnfrba/tp-2019-2c-No-C-Nada.git
#Repo Hilolay: git clone https://github.com/sisoputnfrba/hilolay.git


MYPATH ="/home/utnso/workspace/tp-2019-2c-No-C-Nada"

sudo nano /home/utnso/.profile 

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:"/home/utnso/workspace/tp-2019-2c-No-C-Nada/biblioNOC/Debug":"/home/utnso/workspace/tp-2019-2c-No-C-Nada/biblioSuse/Debug":"/home/utnso/workspace/tp-2019-2c-No-C-Nada/libMUSE/Debug"

echo "Ready - Variable de Entorno"

env | grep LD

#Levanto  .so

cd "$MYPATH/biblioNOC/Debug" 
make clean
make all

cd "$MYPATH/libMUSE/Debug"
make clean
make all

cd "$MYPATH/biblioSuse/Debug"
make clean
make all


#cd tp-2019-2c-No-C-Nada/
#rm hilolay -r
#cd hilolay
#make all
#sudo make install

echo "Realizado - Fin de script"



