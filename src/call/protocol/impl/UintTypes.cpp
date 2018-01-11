//------------------------------------------------------------------------------
/*
    This file is part of calld: https://github.com/call/calld
    Copyright (c) 2014 Call Labs Inc.

    Permission to use, copy, modify, and/or distribute this software for any
    purpose  with  or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE  SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    WITH  REGARD  TO  THIS  SOFTWARE  INCLUDING  ALL  IMPLIED  WARRANTIES  OF
    MERCHANTABILITY  AND  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY  SPECIAL ,  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER  RESULTING  FROM  LOSS  OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
//==============================================================================

#include <BeastConfig.h>
#include <call/protocol/Serializer.h>
#include <call/protocol/SystemParameters.h>
#include <call/protocol/UintTypes.h>
#include <call/protocol/types.h>
#include <call/basics/StringUtilities.h>
namespace call {

/*std::string to_string(Currency const& currency)
{
    // Characters we are willing to allow in the ASCII representation of a
    // three-letter currency code.
    static std::string const allowed_characters =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789"
        "<>(){}[]|?!@#$%^&*";

    if (currency == zero)
        return systemCurrencyCode();

    if (currency == noCurrency())
        return "1";

    static Currency const sIsoBits (
        from_hex_text<Currency>("FFFFFFFFFFFFFFFFFFFFFFFF000000FFFFFFFFFF"));

    if ((currency & sIsoBits).isZero ())
    {
        // The offset of the 3 character ISO code in the currency descriptor
        int const isoOffset = 12;

        std::string const iso(
            currency.data () + isoOffset,
            currency.data () + isoOffset + 3);

        // Specifying the system currency code using ISO-style representation
        // is not allowed.
        if ((iso != systemCurrencyCode()) &&
            (iso.find_first_not_of (allowed_characters) == std::string::npos))
        {
            return iso;
        }
    }

    return strHex (currency.begin (), currency.size ());
}

bool to_currency(Currency& currency, std::string const& code)
{
    if (code.empty () || !code.compare (systemCurrencyCode()))
    {
        currency = zero;
        return true;
    }

    static const int CURRENCY_CODE_LENGTH = 3;
    if (code.size () == CURRENCY_CODE_LENGTH)
    {
        Blob codeBlob (CURRENCY_CODE_LENGTH);

        std::transform (code.begin (), code.end (), codeBlob.begin (), ::toupper);

        Serializer  s;

        s.addZeros (96 / 8);
        s.addRaw (codeBlob);
        s.addZeros (16 / 8);
        s.addZeros (24 / 8);

        s.get160 (currency, 0);
        return true;
    }

    if (40 == code.size ())
        return currency.SetHex (code);

    return false;
}
*/
void offset_find(std::string& src, int& index1, int& index2)
{
	while (src[index1] + src[index1 + 1] == 96)
	{
		index1 = index1 + 2;
	}
	index2 = index1;
	while (src[index2] + src[index2 + 1] != 96)
	{
		index2 = index2 + 2;
	}

}

std::string to_string(Currency const& currency)
{
    // static Currency const sIsoBits    ("FFFFFFFFFFFFFFFFFFFFFFFF000000FFFFFFFFFF");
    static Currency const sExtIsoBits(from_hex_text<Currency> ("FFFFFFFFFFFFFFFFFF000000000000FFFFFFFFFF"));

    // Characters we are willing to allow in the ASCII representation of a
    // three-letter currency code.
    static std::string const allowed_characters =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789"
        "<>(){}[]|?!@#$%^&*";

    if (currency == zero)
        return systemCurrencyCode();

    if (currency == noCurrency())
        return "1";

    if ((currency & sExtIsoBits).isZero ())
    {
        // The offset of the 3 character ISO code in the currency descriptor
        // now use 6 character Ext ISO codes
             std::string cur;
	     int index1,index2;
             index1 = index2 = 0;
             cur = strHex(currency.begin(),currency.size());
             offset_find(cur,index1,index2);
             if(index1 != 0 && index2 != 0)
               {	
	          int size = (index2 - index1)/2;
		  int const isoOffset = 15 - size;
                  std::string const iso(
                  currency.data () + isoOffset,
                  currency.data () + isoOffset + size);
        // Specifying the system currency code using ISO-style representation
        // is not allowed.
                 if ((iso != systemCurrencyCode()) &&
                     (iso.find_first_not_of (allowed_characters) == std::string::npos))
                     {
                        return iso;
                     }
               }
    }

    return strHex (currency.begin (), currency.size ());
}

bool to_currency(Currency& currency, std::string const& code)
{
    if (code.empty () || !code.compare (systemCurrencyCode()))
    {
        currency = zero;
        return true;
    }

    static const int CURRENCY_CODE_LENGTH = 3;
    static const int CURRENCY_EXT_CODE_LENGTH = 6;
    if (code.size () >= CURRENCY_CODE_LENGTH && code.size() <= CURRENCY_EXT_CODE_LENGTH)
    {
     //   Blob codeBlob (code.size());
          Blob codeBlob = strCopy(code);
       // std::transform (code.begin (), code.end (), codeBlob.begin (), ::toupper);

        Serializer  s;

        s.addZeros (15-code.size());
        s.addRaw (codeBlob);
        s.addZeros (16 / 8);
        s.addZeros (24 / 8);

        s.get160 (currency, 0);
        return true;
    }

    if (40 == code.size ())
        return currency.SetHex (code);

    return false;
}
Currency to_currency(std::string const& code)
{
    Currency currency;
    if (!to_currency(currency, code))
        currency = noCurrency();
    return currency;
}

Currency const& callCurrency()
{
    static Currency const currency(0);
    return currency;
}

Currency const& noCurrency()
{
    static Currency const currency(1);
    return currency;
}

Currency const& badCurrency()
{
    static Currency const currency(0x43414c4c00000000);
    return currency;
}

} // call
