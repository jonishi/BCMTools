/*
###################################################################################
#
# BCMTools
#
# Copyright (c) 2011-2014 Institute of Industrial Science, The University of Tokyo.
# All rights reserved.
#
# Copyright (c) 2012-2016 Advanced Institute for Computational Science (AICS), RIKEN.
# All rights reserved.
#
# Copyright (c) 2017 Research Institute for Information Technology (RIIT), Kyushu University.
# All rights reserved.
#
###################################################################################
*/

#include "ConfigBase.h"
#include <iostream>
#include <sstream>


/// コンストラクタ.
ConfigBase::ConfigBase(MPI_Comm comm) : comm(comm)
{
}


/// デストラクタ.
ConfigBase::~ConfigBase()
{
  delete configFile;
}


/// 設定ファイル読み込み.
void ConfigBase::load(const char* file)
{
  int myrank = -1;
  MPI_Comm_rank(comm, &myrank);
  if (myrank == 0) {
    try {
      configFile = new ConfigFile(file);
      parse();
    }
    catch(ConfigFile::file_not_found& e) {
      std::cout << "error: cannot open configfile: " << e.filename << std::endl;
      errorExit("input config file.");
    }
    catch(ConfigFile::key_not_found& e) {
      std::cout << "error: cannot find key: " << e.key << std::endl;
      errorExit("input config file.");
    }
    if (!validate()) errorExit("input config file.");
    broadcastConfigFile(configFile);
  }
  else {
    configFile = new ConfigFile;
    receiveConfigFile(configFile);
    parse();
  }

}


/// ConfigFileオブジェクトの内容をrank0からブロードキャスト.
void ConfigBase::broadcastConfigFile(const ConfigFile* configFile)
{
  std::ostringstream outStr;
  outStr << *configFile;

  int size = outStr.str().size() + 1;
  MPI_Bcast(&size, 1, MPI_INT, 0, comm);

  MPI_Bcast((void*)outStr.str().c_str(), size, MPI_CHAR, 0, comm);
  // constを消すためにキャストが必要
}


/// ConfigFileオブジェクトの内容をrank0から受信.
void ConfigBase::receiveConfigFile(ConfigFile* configFile)
{
  int size;
  MPI_Bcast(&size, 1, MPI_INT, 0, comm);

  char* buffer = new char[size];
  MPI_Bcast(buffer, size, MPI_CHAR, 0, comm);

  std::istringstream inStr(buffer);
  inStr >> *configFile;

  delete[] buffer;
}


/// エラー終了.
void ConfigBase::errorExit(const char* message, int code)
{
  std::cout << "error: " << message << std::endl;
  MPI_Abort(comm, code);
}
