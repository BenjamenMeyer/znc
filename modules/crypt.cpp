/*
 * Copyright (C) 2004-2015 ZNC, see the NOTICE file for details.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//! @author prozac@rottenboy.com
//
// The encryption here was designed to be compatible with mircryption's CBC
// mode.
//
// Latest tested against:
// MircryptionSuite - Mircryption ver 1.19.01 (dll v1.15.01 , mirc v7.32) CBC
// loaded and ready.
//
// TODO:
//
// 1) Encrypt key storage file
// 2) Secure key exchange using pub/priv keys and the DH algorithm
// 3) Some way of notifying the user that the current channel is in "encryption
// mode" verses plain text
// 4) Temporarily disable a target (nick/chan)
//
// NOTE: This module is currently NOT intended to secure you from your shell
// admin.
//       The keys are currently stored in plain text, so anyone with access to
//       your account (or root) can obtain them.
//       It is strongly suggested that you enable SSL between znc and your
//       client otherwise the encryption stops at znc and gets sent to your
//       client in plain text.
//

#include <znc/Chan.h>
#include <znc/User.h>
#include <znc/IRCNetwork.h>

#define REQUIRESSL 1
#define NICK_PREFIX_KEY "[nick-prefix]"

class CCryptMod : public CModule {
    CString NickPrefix() {
        MCString::iterator it = FindNV(NICK_PREFIX_KEY);
        return it != EndNV() ? it->second : "*";
    }

  public:
    MODCONSTRUCTOR(CCryptMod) {
        AddHelpCommand();
        AddCommand("DelKey", static_cast<CModCommand::ModCmdFunc>(
                                 &CCryptMod::OnDelKeyCommand),
                   "<#chan|Nick>", "Remove a key for nick or channel");
        AddCommand("SetKey", static_cast<CModCommand::ModCmdFunc>(
                                 &CCryptMod::OnSetKeyCommand),
                   "<#chan|Nick> <Key>", "Set a key for nick or channel");
        AddCommand("ListKeys", static_cast<CModCommand::ModCmdFunc>(
                                   &CCryptMod::OnListKeysCommand),
                   "", "List all keys");
    }

    ~CCryptMod() override {}

    EModRet OnUserMsg(CString& sTarget, CString& sMessage) override {
        sTarget.TrimPrefix(NickPrefix());

        if (sMessage.TrimPrefix("``")) {
            return CONTINUE;
        }

        MCString::iterator it = FindNV(sTarget.AsLower());

        if (it != EndNV()) {
            CChan* pChan = GetNetwork()->FindChan(sTarget);
            CString sNickMask = GetNetwork()->GetIRCNick().GetNickMask();
            if (pChan) {
                if (!pChan->AutoClearChanBuffer())
                    pChan->AddBuffer(":" + NickPrefix() + _NAMEDFMT(sNickMask) +
                                         " PRIVMSG " + _NAMEDFMT(sTarget) +
                                         " :{text}",
                                     sMessage);
                GetUser()->PutUser(":" + NickPrefix() + sNickMask +
                                       " PRIVMSG " + sTarget + " :" + sMessage,
                                   nullptr, GetClient());
            }

            CString sMsg = MakeIvec() + sMessage;
            sMsg.Encrypt(it->second);
            sMsg.Base64Encode();
            sMsg = "+OK *" + sMsg;

            PutIRC("PRIVMSG " + sTarget + " :" + sMsg);
            return HALTCORE;
        }

        return CONTINUE;
    }

    EModRet OnUserNotice(CString& sTarget, CString& sMessage) override {
        sTarget.TrimPrefix(NickPrefix());

        if (sMessage.TrimPrefix("``")) {
            return CONTINUE;
        }

        MCString::iterator it = FindNV(sTarget.AsLower());

        if (it != EndNV()) {
            CChan* pChan = GetNetwork()->FindChan(sTarget);
            CString sNickMask = GetNetwork()->GetIRCNick().GetNickMask();
            if (pChan) {
                if (!pChan->AutoClearChanBuffer())
                    pChan->AddBuffer(":" + NickPrefix() + _NAMEDFMT(sNickMask) +
                                         " NOTICE " + _NAMEDFMT(sTarget) +
                                         " :{text}",
                                     sMessage);
                GetUser()->PutUser(":" + NickPrefix() + sNickMask + " NOTICE " +
                                       sTarget + " :" + sMessage,
                                   NULL, GetClient());
            }

            CString sMsg = MakeIvec() + sMessage;
            sMsg.Encrypt(it->second);
            sMsg.Base64Encode();
            sMsg = "+OK *" + sMsg;

            PutIRC("NOTICE " + sTarget + " :" + sMsg);
            return HALTCORE;
        }

        return CONTINUE;
    }

    EModRet OnUserAction(CString& sTarget, CString& sMessage) override {
        sTarget.TrimPrefix(NickPrefix());

        if (sMessage.TrimPrefix("``")) {
            return CONTINUE;
        }

        MCString::iterator it = FindNV(sTarget.AsLower());

        if (it != EndNV()) {
            CChan* pChan = GetNetwork()->FindChan(sTarget);
            CString sNickMask = GetNetwork()->GetIRCNick().GetNickMask();
            if (pChan) {
                if (!pChan->AutoClearChanBuffer())
                    pChan->AddBuffer(":" + NickPrefix() + _NAMEDFMT(sNickMask) +
                                         " PRIVMSG " + _NAMEDFMT(sTarget) +
                                         " :\001ACTION {text}\001",
                                     sMessage);
                GetUser()->PutUser(":" + NickPrefix() + sNickMask +
                                       " PRIVMSG " + sTarget + " :\001ACTION " +
                                       sMessage + "\001",
                                   NULL, GetClient());
            }

            CString sMsg = MakeIvec() + sMessage;
            sMsg.Encrypt(it->second);
            sMsg.Base64Encode();
            sMsg = "+OK *" + sMsg;

            PutIRC("PRIVMSG " + sTarget + " :\001ACTION " + sMsg + "\001");
            return HALTCORE;
        }

        return CONTINUE;
    }

    EModRet OnUserTopic(CString& sTarget, CString& sMessage) override {
        sTarget.TrimPrefix(NickPrefix());

        if (sMessage.TrimPrefix("``")) {
            return CONTINUE;
        }

        MCString::iterator it = FindNV(sTarget.AsLower());

        if (it != EndNV()) {
            sMessage = MakeIvec() + sMessage;
            sMessage.Encrypt(it->second);
            sMessage.Base64Encode();
            sMessage = "+OK *" + sMessage;
        }

        return CONTINUE;
    }

    EModRet OnPrivMsg(CNick& Nick, CString& sMessage) override {
        FilterIncoming(Nick.GetNick(), Nick, sMessage);
        return CONTINUE;
    }

    EModRet OnPrivNotice(CNick& Nick, CString& sMessage) override {
        FilterIncoming(Nick.GetNick(), Nick, sMessage);
        return CONTINUE;
    }

    EModRet OnPrivAction(CNick& Nick, CString& sMessage) override {
        FilterIncoming(Nick.GetNick(), Nick, sMessage);
        return CONTINUE;
    }

    EModRet OnChanMsg(CNick& Nick, CChan& Channel, CString& sMessage) override {
        FilterIncoming(Channel.GetName(), Nick, sMessage);
        return CONTINUE;
    }

    EModRet OnChanNotice(CNick& Nick, CChan& Channel,
                         CString& sMessage) override {
        FilterIncoming(Channel.GetName(), Nick, sMessage);
        return CONTINUE;
    }

    EModRet OnChanAction(CNick& Nick, CChan& Channel,
                         CString& sMessage) override {
        FilterIncoming(Channel.GetName(), Nick, sMessage);
        return CONTINUE;
    }

    EModRet OnTopic(CNick& Nick, CChan& Channel, CString& sMessage) override {
        FilterIncoming(Channel.GetName(), Nick, sMessage);
        return CONTINUE;
    }

    EModRet OnRaw(CString& sLine) override {
        if (!sLine.Token(1).Equals("332")) {
            return CONTINUE;
        }

        CChan* pChan = GetNetwork()->FindChan(sLine.Token(3));
        if (pChan) {
            CNick* Nick = pChan->FindNick(sLine.Token(2));
            CString sTopic = sLine.Token(4, true);
            sTopic.TrimPrefix(":");

            FilterIncoming(pChan->GetName(), *Nick, sTopic);
            sLine = sLine.Token(0) + " " + sLine.Token(1) + " " +
                    sLine.Token(2) + " " + pChan->GetName() + " :" + sTopic;
        }

        return CONTINUE;
    }

    void FilterIncoming(const CString& sTarget, CNick& Nick,
                        CString& sMessage) {
        if (sMessage.TrimPrefix("+OK *")) {
            MCString::iterator it = FindNV(sTarget.AsLower());

            if (it != EndNV()) {
                sMessage.Base64Decode();
                sMessage.Decrypt(it->second);
                sMessage.LeftChomp(8);
                sMessage = sMessage.c_str();
                Nick.SetNick(NickPrefix() + Nick.GetNick());
            }
        }
    }

    void OnDelKeyCommand(const CString& sCommand) {
        CString sTarget = sCommand.Token(1);

        if (!sTarget.empty()) {
            if (DelNV(sTarget.AsLower())) {
                PutModule("Target [" + sTarget + "] deleted");
            } else {
                PutModule("Target [" + sTarget + "] not found");
            }
        } else {
            PutModule("Usage DelKey <#chan|Nick>");
        }
    }

    void OnSetKeyCommand(const CString& sCommand) {
        CString sTarget = sCommand.Token(1);
        CString sKey = sCommand.Token(2, true);

        // Strip "cbc:" from beginning of string incase someone pastes directly
        // from mircryption
        sKey.TrimPrefix("cbc:");

        if (!sKey.empty()) {
            SetNV(sTarget.AsLower(), sKey);
            PutModule("Set encryption key for [" + sTarget + "] to [" + sKey +
                      "]");
        } else {
            PutModule("Usage: SetKey <#chan|Nick> <Key>");
        }
    }

    void OnListKeysCommand(const CString& sCommand) {
        if (BeginNV() == EndNV()) {
            PutModule("You have no encryption keys set.");
        } else {
            CTable Table;
            Table.AddColumn("Target");
            Table.AddColumn("Key");

            for (MCString::iterator it = BeginNV(); it != EndNV(); ++it) {
                Table.AddRow();
                Table.SetCell("Target", it->first);
                Table.SetCell("Key", it->second);
            }

            MCString::iterator it = FindNV(NICK_PREFIX_KEY);
            if (it == EndNV()) {
                Table.AddRow();
                Table.SetCell("Target", NICK_PREFIX_KEY);
                Table.SetCell("Key", NickPrefix());
            }

            PutModule(Table);
        }
    }

    CString MakeIvec() {
        CString sRet;
        time_t t;
        time(&t);
        int r = rand();
        sRet.append((char*)&t, 4);
        sRet.append((char*)&r, 4);

        return sRet;
    }
};

template <>
void TModInfo<CCryptMod>(CModInfo& Info) {
    Info.SetWikiPage("crypt");
}

NETWORKMODULEDEFS(CCryptMod, "Encryption for channel/private messages")
