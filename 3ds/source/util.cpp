/*
*   This file is part of Checkpoint
*   Copyright (C) 2017-2018 Bernardo Giordano
*
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*   Additional Terms 7.b and 7.c of GPLv3 apply to this file:
*       * Requiring preservation of specified reasonable legal notices or
*         author attributions in that material or in the Appropriate Legal
*         Notices displayed by works containing it.
*       * Prohibiting misrepresentation of the origin of that material,
*         or requiring that modified versions of such material be marked in
*         reasonable ways as different from the original version.
*/

#include "util.hpp"

void servicesExit(void)
{
    Gui::exit();
    pxiDevExit();
    Archive::exit();
    amExit();
    srvExit();
    hidExit();
    sdmcExit();
    romfsExit();
}

void servicesInit(void)
{
    Result res = 0;
    romfsInit();
    sdmcInit();
    hidInit();
    srvInit();
    amInit();
    pxiDevInit();
    
    res = Archive::init();
    if (R_FAILED(res))
    {
        Gui::createError(res, "SDMC archive init failed.");
    }
    
    mkdir("sdmc:/3ds", 777);
    mkdir("sdmc:/3ds/Checkpoint", 777);
    mkdir("sdmc:/3ds/Checkpoint/saves", 777);
    mkdir("sdmc:/3ds/Checkpoint/extdata", 777);
    
    Gui::init();

    // consoleDebugInit(debugDevice_SVC);
    // while (aptMainLoop() && !(hidKeysDown() & KEY_START)) { hidScanInput(); }

    Configuration::getInstance();
}

void calculateTitleDBHash(u8* hash)
{
    u32 titleCount, titlesRead;
    AM_GetTitleCount(MEDIATYPE_SD, &titleCount);
    u64 buf[titleCount + 1];
    AM_GetTitleList(&titlesRead, MEDIATYPE_SD, titleCount, buf);
    
    u64 cardID;
    FS_CardType cardType;
    Result res = FSUSER_GetCardType(&cardType);
    if (R_FAILED(res))
    {
        buf[titleCount] = 0xFFFFFFFFFFFFFFFF;
    }
    else if (cardType == CARD_CTR)
    {
        AM_GetTitleList(NULL, MEDIATYPE_GAME_CARD, 1, &cardID);
        buf[titleCount] = cardID;
    }
    else
    {
        u8 headerData[0x3B4];
        Result res = FSUSER_GetLegacyRomHeader(MEDIATYPE_GAME_CARD, 0LL, headerData);
        if (R_SUCCEEDED(res))
        {
            memcpy(&cardID, headerData + 3, sizeof(u64));
            buf[titleCount] = cardID;
        }
        else
        {
            buf[titleCount] = 0LL;
        }
    }
    
    sha256(hash, (u8*)buf, (titleCount + 1) * sizeof(u64));
}

std::u16string StringUtils::UTF8toUTF16(const char* src)
{
    char16_t tmp[256] = {0};
    utf8_to_utf16((uint16_t *)tmp, (uint8_t *)src, 256);
    return std::u16string(tmp);
}

std::u16string StringUtils::removeForbiddenCharacters(std::u16string src)
{
    static const std::u16string illegalChars = StringUtils::UTF8toUTF16(".,!\\/:?*\"<>|");
    for (size_t i = 0; i < src.length(); i++)
    {
        if (illegalChars.find(src[i]) != std::string::npos)
        {
            src[i] = ' ';
        }
    }
    
    size_t i;
    for (i = src.length() - 1; i > 0 && src[i] == L' '; i--);
    src.erase(i + 1, src.length() - i);

    return src;
}