// ==========================================================================
//         ____            _                     _____           _
//        / ___| _   _ ___| |_ ___ _ __ ___     |_   _|__   ___ | |___
//        \___ \| | | / __| __/ _ \ '_ ` _ \ _____| |/ _ \ / _ \| / __|
//         ___) | |_| \__ \ ||  __/ | | | | |_____| | (_) | (_) | \__ \.
//        |____/ \__, |___/\__\___|_| |_| |_|     |_|\___/ \___/|_|___/
//               |___/
//                             --- System-Tools ---
//                  https://www.nntb.no/~dreibh/system-tools/
// ==========================================================================
//
// System-Info Library
// Copyright (C) 2024-2026 by Thomas Dreibholz
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// Contact: thomas.dreibholz@gmail.com

#include <libsysteminfo.h>

#include <algorithm>
#include <string>
#include <vector>

#if !defined(_WIN32)
#include <grp.h>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#else
#include <windows.h>
#include <sddl.h>
#endif


// ##### Obtain user information ############################################
void SystemInfo::obtainUserInformation()
{
#if !defined(_WIN32)
   const uid_t userIdentifier  = getuid();
   const gid_t groupIdentifier = getgid();

   addUInt32("user_uid", static_cast<uint32_t>(userIdentifier));
   addUInt32("user_gid", static_cast<uint32_t>(groupIdentifier));

   const struct passwd* passwordEntry = getpwuid(userIdentifier);
   if(passwordEntry != nullptr) {
      if(passwordEntry->pw_name != nullptr) {
         addString("user_name", passwordEntry->pw_name);
      }
      else {
         addString("user_name", "");
      }

      std::string fullName;
      if(passwordEntry->pw_gecos != nullptr) {
         fullName = passwordEntry->pw_gecos;
         const auto commaPosition = fullName.find(',');
         if(commaPosition != std::string::npos) {
            fullName = fullName.substr(0, commaPosition);
         }
      }
      addString("user_fullname", fullName);
   }
   else {
      addString("user_name", "");
      addString("user_fullname", "");
   }

   const struct group* primaryGroupEntry = getgrgid(groupIdentifier);
   if((primaryGroupEntry != nullptr) && (primaryGroupEntry->gr_name != nullptr)) {
      addString("user_group", primaryGroupEntry->gr_name);
   }
   else {
      addString("user_group", std::to_string(groupIdentifier));
   }

   std::vector<gid_t> groupIdentifiers;
   groupIdentifiers.push_back(groupIdentifier);

   const int numberOfGroups = getgroups(0, nullptr);
   if(numberOfGroups > 0) {
      const size_t       groupsSize = static_cast<size_t>(numberOfGroups);
      std::vector<gid_t> supplementaryGroups(groupsSize);
      const int returnedGroups = getgroups(numberOfGroups, supplementaryGroups.data());
      if(returnedGroups > 0) {
         const size_t returnedSize = static_cast<size_t>(returnedGroups);
         for(size_t index = 0; index < returnedSize; ++index) {
            if(std::find(groupIdentifiers.begin(),
                         groupIdentifiers.end(),
                         supplementaryGroups[index]) == groupIdentifiers.end()) {
               groupIdentifiers.push_back(supplementaryGroups[index]);
            }
         }
      }
   }

   std::string groupList;
   for(const auto currentGroupIdentifier : groupIdentifiers) {
      const struct group* groupEntry = getgrgid(currentGroupIdentifier);
      std::string groupName;
      if((groupEntry != nullptr) && (groupEntry->gr_name != nullptr)) {
         groupName = groupEntry->gr_name;
      }
      else {
         groupName = std::to_string(currentGroupIdentifier);
      }

      if(!groupList.empty()) {
         groupList += ' ';
      }
      groupList += groupName;
   }
   addString("user_groups", groupList);

#else
   char  userNameBuffer[256] { 0 };
   DWORD userNameSize = sizeof(userNameBuffer);
   if(GetUserNameA(userNameBuffer, &userNameSize)) {
      addString("user_name", userNameBuffer);
   }
   else {
      addString("user_name", "unknown");
   }

   HANDLE      tokenHandle     { nullptr };
   DWORD       userIdentifier  { 0 };
   DWORD       groupIdentifier { 0 };
   std::string primaryGroupName;
   std::string groupList;

   if(OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &tokenHandle)) {
      DWORD requiredSize { 0 };
      GetTokenInformation(tokenHandle, TokenUser, nullptr, 0, &requiredSize);
      if(requiredSize > 0) {
         std::vector<BYTE> tokenUserBuffer(requiredSize);
         if(GetTokenInformation(tokenHandle,
                                TokenUser,
                                tokenUserBuffer.data(),
                                requiredSize,
                                &requiredSize)) {
            auto* tokenUser = reinterpret_cast<TOKEN_USER*>(
               tokenUserBuffer.data());
            if(IsValidSid(tokenUser->User.Sid)) {
               const DWORD subAuthorityCount = *GetSidSubAuthorityCount(
                  tokenUser->User.Sid);
               if(subAuthorityCount > 0) {
                  userIdentifier = *GetSidSubAuthority(
                     tokenUser->User.Sid, subAuthorityCount - 1);
               }
            }
         }
      }

      requiredSize = 0;
      GetTokenInformation(tokenHandle,
                          TokenPrimaryGroup,
                          nullptr,
                          0,
                          &requiredSize);
      if(requiredSize > 0) {
         std::vector<BYTE> tokenGroupBuffer(requiredSize);
         if(GetTokenInformation(tokenHandle,
                                TokenPrimaryGroup,
                                tokenGroupBuffer.data(),
                                requiredSize,
                                &requiredSize)) {
            auto* tokenPrimaryGroup = reinterpret_cast<TOKEN_PRIMARY_GROUP*>(
               tokenGroupBuffer.data());
            if(IsValidSid(tokenPrimaryGroup->PrimaryGroup)) {
               const DWORD subAuthorityCount = *GetSidSubAuthorityCount(
                  tokenPrimaryGroup->PrimaryGroup);
               if(subAuthorityCount > 0) {
                  groupIdentifier = *GetSidSubAuthority(
                     tokenPrimaryGroup->PrimaryGroup, subAuthorityCount - 1);
               }

               char  primaryGroupBuffer[256] { 0 };
               char  domainName[256]         { 0 };
               DWORD primaryGroupSize        = sizeof(primaryGroupBuffer);
               DWORD domainNameSize          = sizeof(domainName);
               SID_NAME_USE sidNameUse;
               if(LookupAccountSidA(nullptr,
                                    tokenPrimaryGroup->PrimaryGroup,
                                    primaryGroupBuffer,
                                    &primaryGroupSize,
                                    domainName,
                                    &domainNameSize,
                                    &sidNameUse)) {
                  primaryGroupName = primaryGroupBuffer;
               }
            }
         }
      }

      requiredSize = 0;
      GetTokenInformation(tokenHandle, TokenGroups, nullptr, 0, &requiredSize);
      if(requiredSize > 0) {
         std::vector<BYTE> tokenGroupsBuffer(requiredSize);
         if(GetTokenInformation(tokenHandle,
                                TokenGroups,
                                tokenGroupsBuffer.data(),
                                requiredSize,
                                &requiredSize)) {
            auto* tokenGroups = reinterpret_cast<TOKEN_GROUPS*>(
               tokenGroupsBuffer.data());
            for(DWORD index = 0; index < tokenGroups->GroupCount; ++index) {
               char  groupName[256]  { 0 };
               char  domainName[256] { 0 };
               DWORD groupNameSize   = sizeof(groupName);
               DWORD domainNameSize  = sizeof(domainName);
               SID_NAME_USE sidNameUse;
               if(LookupAccountSidA(nullptr,
                                    tokenGroups->Groups[index].Sid,
                                    groupName,
                                    &groupNameSize,
                                    domainName,
                                    &domainNameSize,
                                    &sidNameUse)) {
                  if((sidNameUse == SidTypeGroup) ||
                     (sidNameUse == SidTypeWellKnownGroup) ||
                     (sidNameUse == SidTypeAlias)) {
                     if(!groupList.empty()) {
                        groupList += ' ';
                     }
                     groupList += groupName;
                  }
               }
            }
         }
      }

      CloseHandle(tokenHandle);
   }

   addUInt32("user_uid", userIdentifier);
   addUInt32("user_gid", groupIdentifier);
   addString("user_group", primaryGroupName.empty() ? std::to_string(groupIdentifier) : primaryGroupName);
   addString("user_groups", groupList);

   char  fullNameBuffer[256] { 0 };
   ULONG fullNameSize = sizeof(fullNameBuffer);
   HMODULE secur32Module = LoadLibraryA("secur32.dll");
   if(secur32Module != nullptr) {
      using fnGetUserNameExA = BOOLEAN(WINAPI*)(int, LPSTR, PULONG);
      auto pGetUserNameExA = reinterpret_cast<fnGetUserNameExA>(
         GetProcAddress(secur32Module, "GetUserNameExA"));
      if(pGetUserNameExA && pGetUserNameExA(3, fullNameBuffer, &fullNameSize)) {
         addString("user_fullname", fullNameBuffer);
      }
      else {
         addString("user_fullname", userNameBuffer);
      }
      FreeLibrary(secur32Module);
   }
   else {
      addString("user_fullname", userNameBuffer);
   }
#endif
}
