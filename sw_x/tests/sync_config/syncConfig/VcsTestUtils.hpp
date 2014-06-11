#ifndef TESTVCSPROXY_HPP_
#define TESTVCSPROXY_HPP_

#include "vcs_util.hpp"
#include <string>

// removeContentsOnly - When set to true, removes only the contents
//                      of the folder and not folder itself.
int vcsRmFolderRecursive(const VcsSession& vcsSession,
                         const VcsDataset& dataset,
                         const std::string& folderArg,
                         u64 compId,
                         bool removeContentsOnly,
                         bool printLog);

#endif /* TESTVCSPROXY_HPP_ */
