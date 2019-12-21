#! /bin/bash -e
#Flag error -> cierre del script

#Repo tp-2019-2c-No-C-Nada: git clone https://github.com/sisoputnfrba/tp-2019-2c-No-C-Nada.git
#Repo Hilolay: git clone https://github.com/sisoputnfrba/hilolay.git


MYPATH="/home/utnso/workspace/tp-2019-2c-No-C-Nada"

 

export LD_LIBRARY_PATH=/home/utnso/workspace/tp-2019-2c-No-C-Nada/biblioNOC/Debug:/home/utnso/workspace/tp-2019-2c-No-C-Nada/biblioSuse/Debug:/home/utnso/workspace/tp-2019-2c-No-C-Nada/libMUSE/Debug

echo "Ready - Variable de Entorno"



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

cd "$MYPATH/MUSE/Debug"
make clean
make all


echo "Realizado - Fin de script"




