/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include "fairdicetoken.hpp"

namespace eosico {



    void ico::create( account_name issuer,
                      asset        maximum_supply )
    {
        require_auth( _self );

        auto sym = maximum_supply.symbol;
        eosio_assert( sym.is_valid(), "invalid symbol name" );
        eosio_assert( maximum_supply.is_valid(), "invalid supply");
        eosio_assert( maximum_supply.amount > 0, "max-supply must be positive");

        eosio_assert( maximum_supply.symbol == TOKEN_SYMBOL, "only can create MMT symbol");

        stats statstable( _self, sym.name() );
        auto existing = statstable.find( sym.name() );
        eosio_assert( existing == statstable.end(), "token with symbol already exists" );

        statstable.emplace( _self, [&]( auto& s ) {
            s.supply.symbol = maximum_supply.symbol;
            s.max_supply    = maximum_supply;
            s.issuer        = issuer;
        });
    }


    void ico::issue( account_name to, asset quantity, string memo )
    {
        auto sym = quantity.symbol;
        eosio_assert( sym.is_valid(), "invalid symbol name" );
        eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

        auto sym_name = sym.name();
        stats statstable( _self, sym_name );
        auto existing = statstable.find( sym_name );
        eosio_assert( existing != statstable.end(), "token with symbol does not exist, create token before issue" );
        const auto& st = *existing;

        require_auth( st.issuer );
        eosio_assert( quantity.is_valid(), "invalid quantity" );
        eosio_assert( quantity.amount > 0, "must issue positive quantity" );

        eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
        eosio_assert( quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

        statstable.modify( st, 0, [&]( auto& s ) {
            s.supply += quantity;
        });

        add_balance( st.issuer, quantity, st.issuer );

        if( to != st.issuer ) {
            SEND_INLINE_ACTION( *this, transfer, {st.issuer,N(active)}, {st.issuer, to, quantity, memo} );
        }

        globals _global( _self, _self );
        global global = _global.get_or_default();
        global.mine.symbol = quantity.symbol;
        global.mine.amount += quantity.amount;
        _global.set(global, _self);
    }

    void ico::issuelock( account_name to, asset quantity, string memo, uint64_t account_group )
    {
        auto sym = quantity.symbol;
        eosio_assert( sym.is_valid(), "invalid symbol name" );
        eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

        auto sym_name = sym.name();
        eosio::print("sym_name",sym_name);
        stats statstable( _self, sym_name );
        auto existing = statstable.find( sym_name );
        eosio_assert( existing != statstable.end(), "token with symbol does not exist, create token before issue" );
        const auto& st = *existing;

        require_auth( st.issuer );
        eosio_assert( quantity.is_valid(), "invalid quantity" );
        eosio_assert( quantity.amount > 0, "must issue positive quantity" );

        eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
        eosio_assert( quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

        statstable.modify( st, 0, [&]( auto& s ) {
            s.supply += quantity;
        });

        add_balance( st.issuer, quantity, st.issuer );

        if( to != st.issuer ) {
            SEND_INLINE_ACTION( *this, transferlock, {st.issuer,N(active)}, {st.issuer, to, quantity, memo, account_group} );
        }
    }

    void ico::issueunlock( account_name owner)
    {
        auto sym_name = symbol_type(TOKEN_SYMBOL).name();
        stats statstable( _self, sym_name );
        auto existing = statstable.find( sym_name );
        eosio::print(existing->supply);
        eosio_assert( existing != statstable.end(), "token with symbol does not exist" );
        const auto& st = *existing;
        require_auth( st.issuer );

        globals _global( _self, _self );
        global global = _global.get_or_default();
        asset mine = global.mine;

        asset quantity = asset(0, TOKEN_SYMBOL);
        asset value = asset(0, TOKEN_SYMBOL);
        allaccounts _allaccounts(_self, sym_name);
        const auto& itr = _allaccounts.get( owner, "no balance object found" );
        switch(itr.account_group){
            case TEAM_ACCOUNT_GROUP:
                value = global.mine * TEAM_UNLOCK / 100;
                if(value <= itr.lock_balance ){
                    quantity = value;
                }
                else{
                    quantity = itr.lock_balance;
                }
                break;
            case ICO_ACCOUNT_GROUP:
                value = global.mine * ICO_UNLOCK / 100;
                if(value <= itr.lock_balance ){
                    quantity = value;
                }
                else{
                    quantity = itr.lock_balance;
                }
                break;
            case TUIGUANG_ACCOUNT_GROUP:
                value = global.mine * TUIGUANG_UNLOCK / 100;
                if(value <= itr.lock_balance ){
                    quantity = value;
                }
                else{
                    quantity = itr.lock_balance;
                }
                break;
            default:
                eosio::print("no account group");
                break;
        }

        eosio_assert( quantity.is_valid(), "invalid quantity" );
        eosio_assert( quantity.amount > 0, "must issue positive quantity" );

        eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );

        accounts from_acnts( _self, owner );

        const auto& from = from_acnts.get( quantity.symbol.name(), "no balance object found" );
        eosio_assert( from.lock_balance.amount >= quantity.amount, "overdrawn balance" );

        _allaccounts.modify( itr, 0, [&]( auto& a ) {
            a.lock_balance -= quantity;
        });

        from_acnts.modify( from, 0, [&]( auto& a ) {
            a.lock_balance -= quantity;
        });

    }
    void ico::retire( asset quantity, string memo )
    {
        auto sym = quantity.symbol;
        eosio_assert( sym.is_valid(), "invalid symbol name" );
        eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

        auto sym_name = sym.name();
        stats statstable( _self, sym_name );
        auto existing = statstable.find( sym_name );
        eosio_assert( existing != statstable.end(), "token with symbol does not exist" );
        const auto& st = *existing;

        require_auth( st.issuer );
        eosio_assert( quantity.is_valid(), "invalid quantity" );
        eosio_assert( quantity.amount > 0, "must retire positive quantity" );

        eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );

        statstable.modify( st, 0, [&]( auto& s ) {
            s.supply -= quantity;
        });

        sub_balance( st.issuer, quantity );
    }

    void ico::transfer( account_name from,
                        account_name to,
                        asset        quantity,
                        string       memo )
    {
        require_auth( from );

        eosio_assert( from != to, "cannot transfer to self" );
        eosio_assert( is_account( to ), "to account does not exist");
        auto sym = quantity.symbol.name();
        stats statstable( _self, sym );
        const auto& st = statstable.get( sym );

        require_recipient( from );
        require_recipient( to );

        eosio_assert( quantity.is_valid(), "invalid quantity" );
        eosio_assert( quantity.amount > 0, "must transfer positive quantity" );
        eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
        eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

        sub_balance(from, quantity);
        add_balance(to, quantity, from);
    }

    void ico::sub_balance( account_name owner, asset value ) {
        accounts from_acnts( _self, owner );

        const auto& from = from_acnts.get( value.symbol.name(), "no balance object found" );
        eosio_assert( (from.balance.amount - from.lock_balance.amount) >= value.amount, "overdrawn balance" );

        allaccounts _allaccounts(_self, value.symbol.name());
        const auto& itr = _allaccounts.get( owner, "no balance object found" );
        if(from.balance.amount == value.amount){
            _allaccounts.erase(itr);
        }else{
            _allaccounts.modify( itr, owner, [&]( auto& a ) {
                a.balance -= value;
            });
        }

        if( from.balance.amount == value.amount ) {
            from_acnts.erase( from );
        } else {
            from_acnts.modify( from, owner, [&]( auto& a ) {
                a.balance -= value;
            });
        }
    }

    void ico::add_balance( account_name owner, asset value, account_name ram_payer )
    {
        accounts to_acnts( _self, owner );
        auto to = to_acnts.find( value.symbol.name() );

        allaccounts _allaccounts(_self, value.symbol.name());
        auto itr = _allaccounts.find(owner);
        if(itr == _allaccounts.end()){
            _allaccounts.emplace( ram_payer, [&]( auto& a ){
                a.account = owner;
                a.balance = value;
                a.lock_balance = asset(0, value.symbol);;
            });
        }else if(itr != _allaccounts.end()){
            _allaccounts.modify( itr, 0, [&]( auto& a ) {
                a.balance += value;
            });
        }

        if( to == to_acnts.end() ) {
            to_acnts.emplace( ram_payer, [&]( auto& a ){
                a.balance = value;
                a.lock_balance = asset(0, value.symbol);
            });
        } else {
            to_acnts.modify( to, 0, [&]( auto& a ) {
                a.balance += value;
            });
        }
    }

    void ico::transferlock( account_name from,
                        account_name to,
                        asset        quantity,
                        string       memo,
                        uint64_t account_group)
    {
        require_auth( from );

        eosio_assert( from != to, "cannot transfer to self" );
        eosio_assert( is_account( to ), "to account does not exist");
        auto sym = quantity.symbol.name();
        stats statstable( _self, sym );
        const auto& st = statstable.get( sym );

        require_recipient( from );
        require_recipient( to );

        eosio_assert( quantity.is_valid(), "invalid quantity" );
        eosio_assert( quantity.amount > 0, "must transfer positive quantity" );
        eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
        eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );


        sub_balance( from, quantity );
        add_balance_lock( to, quantity, from, account_group);
    }

    void ico::add_balance_lock( account_name owner, asset value, account_name ram_payer, uint64_t account_group )
    {
        accounts to_acnts( _self, owner );
        auto to = to_acnts.find( value.symbol.name() );

        allaccounts _allaccounts(_self, value.symbol.name());
        auto itr = _allaccounts.find(owner);
        if(itr == _allaccounts.end()){
            _allaccounts.emplace( ram_payer, [&]( auto& a ){
                a.account = owner;
                a.balance = value;
                a.lock_balance = value;
                a.account_group = account_group;
            });
        }else if(itr != _allaccounts.end()){
            _allaccounts.modify( itr, 0, [&]( auto& a ) {
                a.balance += value;
                a.lock_balance += value;
                a.account_group = account_group;
            });
        }


        if( to == to_acnts.end() ) {
            to_acnts.emplace( ram_payer, [&]( auto& a ){
                a.balance = value;
                a.lock_balance = value;
            });
        } else {
            to_acnts.modify( to, 0, [&]( auto& a ) {
                a.balance += value;
                a.lock_balance += value;
            });
        }
    }

    void ico::buykey(account_name to, asset quantity, string memo)
    {
        require_auth(to);
        eosio_assert(quantity.symbol == S(4, EOS), "symbol must be EOS");

        stats statstable( _self, symbol_type(TOKEN_SYMBOL).name() );
        auto existing = statstable.find( symbol_type(TOKEN_SYMBOL).name() );
        eosio_assert( existing != statstable.end(), "token with symbol does not exist, create token before issue" );

        eosio_assert( existing->supply.amount+quantity.amount <= PHASE_ONE, "the key has been sold more than 1000000000.0000");

        SEND_INLINE_ACTION( *this, issue, {existing->issuer,N(active)}, {to, asset{quantity.amount, TOKEN_SYMBOL}, memo} );

        icoinfos _icoinfo(_self, symbol_type(TOKEN_SYMBOL).name());
        _icoinfo.emplace( _self, [&]( auto& a ){
            a.id = _icoinfo.available_primary_key();
            a.account = to;
            a.quant = quantity;
            a.time = time_point_sec(now());
        });
    }

    void ico::close( account_name owner, string symbol)
    {
        symbol_type sym = symbol_type(S(4,symbol.c_str()));
        accounts acnts( _self, owner );
        auto it = acnts.find( sym.name()  );
        eosio_assert( it != acnts.end(), "Balance row already deleted or never existed. Action won't have any effect." );
        eosio_assert( it->balance.amount == 0, "Cannot close because the balance is not zero." );
        acnts.erase( it );
    }

    void ico::destroytoken( string symbol )
    {
        require_auth( _self );
        symbol_type sym = string_to_symbol(4, symbol.c_str());
        stats statstable( _self, sym.name() );
        auto existing = statstable.find( sym.name() );
        eosio_assert( existing != statstable.end(), "token with symbol does not exist" );

        statstable.erase( existing );
    }

    void ico::destroyacc( string symbol, account_name acc)
    {
        require_auth( _self );
        symbol_type sym = string_to_symbol(4, symbol.c_str());
        accounts acctable( _self, acc );
        const auto& row = acctable.get( sym.name(), "no balance object found for provided account and symbol" );
        acctable.erase( row );
    }

    void ico::clear(string table, uint32_t numbers, account_name owner, string symbol_name )
    {
        require_auth(_self);
        symbol_type symbol = symbol_type(TOKEN_SYMBOL);
        uint32_t count = 0 ;
        string tb_accounts = "accounts";
        string tb_stats = "stats";
        string tb_allaccounts = "allaccounts";
        string tb_icoinfos = "icoinfos";
        string tb_globals = "globals";
        if(table == tb_accounts){
            accounts acnts( _self, owner );
            //empty bets table
            for(auto itr = acnts.begin(); itr != acnts.end() && count <= numbers;) {
                itr = acnts.erase(itr);
                count++;
            }
        }
        else if(table == tb_stats){
            stats _stats( _self, symbol.name() );
            eosio::print(symbol.name());
            //empty hash table
            for(auto itr = _stats.begin(); itr != _stats.end() && count <= numbers;) {
                itr = _stats.erase(itr);
                count++;
            }
        }
        else if(table == tb_allaccounts){
            allaccounts _allaccounts(_self,  symbol.name() );
            for(auto itr = _allaccounts.begin(); itr != _allaccounts.end() && count <= numbers;) {
                itr = _allaccounts.erase(itr);
                count++;
            }
        }
        else if(table == tb_icoinfos){
            icoinfos _icoinfo(_self, symbol.name() );
            for(auto itr = _icoinfo.begin(); itr != _icoinfo.end() && count <= numbers;) {
                itr = _icoinfo.erase(itr);
                count++;
            }
        }else if(table == tb_globals){
            globals _global( _self, _self );
            _global.remove();
        }
        else{
            eosio_assert(0, "wrong tables");
        }

    }

} /// namespace eosio


