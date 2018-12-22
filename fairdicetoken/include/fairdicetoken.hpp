/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>
#include <eosiolib/time.hpp>
#include <eosiolib/action.h>
#include "eosio.token.hpp"
#include "types.hpp"
#include <string>

namespace eosiosystem {
    class system_contract;
}

namespace eosico {
    using namespace eosio;
    using std::string;

    class ico : public contract {
    public:
        ico( account_name self ):contract(self){}

        // @abi action
        void create( account_name issuer,
                     asset        maximum_supply);

        // @abi action
        void issue( account_name to, asset quantity, string memo );

        // @abi action
        void retire( asset quantity, string memo );

        // @abi action
        void transfer( account_name from,
                       account_name to,
                       asset        quantity,
                       string       memo );

        void buykey(account_name to, asset quantity, string memo);

        // @abi action
        void close( account_name owner, symbol_type symbol );

        inline asset get_supply( symbol_name sym )const;

        inline asset get_balance( account_name owner, symbol_name sym )const;

    private:
        void sub_balance( account_name owner, asset value );
        void add_balance( account_name owner, asset value, account_name ram_payer );

    public:
        struct transfer_args {
            account_name  from;
            account_name  to;
            asset         quantity;
            string        memo;
        };
    };

    asset ico::get_supply( symbol_name sym )const
    {
        stats statstable( _self, sym );
        const auto& st = statstable.get( sym );
        return st.supply;
    }

    asset ico::get_balance( account_name owner, symbol_name sym )const
    {
        accounts accountstable( _self, owner );
        const auto& ac = accountstable.get( sym );
        return ac.balance;
    }

} /// namespace eosio

using namespace eosico;

extern "C" {

void apply( uint64_t receiver, uint64_t code, uint64_t action ) {
//    print( "\nicoapply,", name{receiver},"\n" );

    auto self = receiver;
    eosico::ico thiscontract( receiver );
    if( code == N(eosio.token) &&  action == N(transfer) ) {
        eosio::token::transfer_args tmp = unpack_action_data<eosio::token::transfer_args>();

        if(tmp.to != self){
            return;
        }

        string memo = tmp.memo;

        memo.erase(memo.begin(), find_if(memo.begin(), memo.end(), [](int ch) {
            return !isspace(ch);
        }));
        memo.erase(find_if(memo.rbegin(), memo.rend(), [](int ch) {
            return !isspace(ch);
        }).base(), memo.end());

        auto separator_pos = memo.find(' ');
        if (separator_pos == string::npos) {
            separator_pos = memo.find(':');
        }

        eosio_assert(separator_pos != string::npos, "Function name and other command must be separated with space or colon");

        string func_name_str = memo.substr(0, separator_pos);
        if(0 == func_name_str.compare("buykey")){
            // TODO: can be opened if token can be bought.
            //thiscontract.buykey(tmp.from, tmp.quantity,tmp.memo);
        }else{
            eosio_assert(false, "the memo format is error");
        }

    }
    else if (code == self  || action == N(onerror) ){
        switch (action)
        {
            EOSIO_API( eosico::ico, (create)(issue)(retire)(transfer)(close) )
        }
    }
}
}

//EOSIO_ABI( eosico::ico, (create)(issue)(transfer) )