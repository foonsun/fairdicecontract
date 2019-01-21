#include "fairdicegame.hpp"

void fairdicegame::reveal(const uint64_t& id, const checksum256& seed) {
    require_auth(REVEALER);
    st_bet bet = find_or_error(id);
    assert_seed(seed, bet.seed_hash);

    uint8_t random_roll = compute_random_roll(seed, bet.user_seed_hash);
    asset payout = asset(0, bet.amount.symbol);
    if (random_roll < bet.roll_under) {
        payout = compute_payout(bet.roll_under, bet.amount);
        action(permission_level{_self, N(active)},
               bet.amount.contract,
               N(transfer),
               make_tuple(_self,
                          bet.player,
                          payout,
                          winner_memo(bet)))
                .send();
        // defer transfer for safe
//        send_defer_action(permission_level{_self, N(active)},
//                          bet.amount.contract,
//                          N(transfer),
//                          make_tuple(_self,
//                                  bet.player,
//                                  payout,
//                                  winner_memo(bet)));
    }
    if (iseostoken(bet.amount))
    {
        issue_token(bet.player, bet.amount, "mining! ");
        unlock(bet.amount);
    }
    if (bet.referrer != _self) {
        // defer trx, no need to rely heavily
        action(permission_level{_self, N(active)},
               bet.amount.contract,
               N(transfer),
               make_tuple(_self,
                          bet.referrer,
                          compute_referrer_reward(bet),
                          referrer_memo(bet)))
                .send();
//        send_defer_action(permission_level{_self, N(active)},
//                          bet.amount.contract,
//                          N(transfer),
//                          make_tuple(_self,
//                                     bet.referrer,
//                                     compute_referrer_reward(bet),
//                                     referrer_memo(bet)));
    }
    action(permission_level{_self, N(active)},
           bet.amount.contract,
           N(transfer),
           make_tuple(_self,
                      DEV,
                      compute_dev_reward(bet),
                      std::string("for dev")))
            .send();
    /*
    send_defer_action(permission_level{_self, N(active)},
                      bet.amount.contract,
                      N(transfer),
                      make_tuple(_self,
                                 DEV,
                                 compute_dev_reward(bet),
                                 std::string("for dev"))));
    */
    //remove(bet);
    remove_ex(id);
    st_result result{.bet_id = bet.id,
                     .player = bet.player,
                     .referrer = bet.referrer,
                     .amount = bet.amount,
                     .roll_under = bet.roll_under,
                     .seed = seed,
                     .seed_hash = bet.seed_hash,
                     .user_seed_hash = bet.user_seed_hash,
                     .payout = payout};
    /*
    send_defer_action(permission_level{_self, N(active)},
                      LOG,
                      N(result),
                      result);
    */
    action(permission_level{_self, N(active)},
           _self,
           N(result),
           result)
            .send();
    /*
    send_defer_action(permission_level{_self, N(active)},
                      _self,
                      N(result),
                      result);
    */
}

void fairdicegame::transfer(const account_name& from,
                            const account_name& to,
                            const extended_asset& quantity,
                            const string& memo) {
    if (from == _self || to != _self) {
        return;
    }
    if ("Transfer bonus" == memo) {
        return;
    }
    check_account1(from);
    uint8_t roll_under;
    checksum256 seed_hash;
    checksum160 user_seed_hash;
    uint64_t expiration;
    account_name referrer;
    signature sig;
    parse_memo(memo, &roll_under, &seed_hash, &user_seed_hash, &expiration, &referrer, &sig);

    //check quantity
    assert_quantity(quantity);

    //check roll_under
    assert_roll_under(roll_under, quantity);

    //check seed hash && expiration
    assert_hash(seed_hash, expiration);

    //check referrer
    eosio_assert(is_account(referrer), "referrer account does not exist");
    eosio_assert(referrer != from, "referrer can not be self");

    //check signature
    assert_signature(roll_under, seed_hash, expiration, referrer, sig);
    const st_bet _bet{.id = next_id(),
                      .player = from,
                      .referrer = referrer,
                      .amount = quantity,
                      .roll_under = roll_under,
                      .seed_hash = seed_hash,
                      .user_seed_hash = user_seed_hash,
                      .created_at = uint64_t(now())*1000,
                      .expiration = expiration};
    save(_bet);
    if (iseostoken(quantity))
    {
        //count player
        iplay(from, quantity);
        lock(quantity);
    }
    action(permission_level{_self, N(active)},
           _bet.amount.contract,
           N(transfer),
           std::make_tuple(_self, DIVIDEND, compute_divpool_reward(_bet), std::string("Dividing pool")))
            .send();
    action(permission_level{_self, N(active)},
           _self,
           N(receipt),
           _bet)
        .send();
}

void fairdicegame::receipt(const st_bet& bet) {
    require_auth(_self);
}

void fairdicegame::result(const st_result& result) {
    require_auth(_self);
    require_recipient(result.player);
}

void fairdicegame::equity(const asset& quantity) {
    require_auth(REVEALER);

    /*
    //check quantity
    assert_quantity(quantity);

    asset payout = asset(0, EOS_SYMBOL);

    payout = compute_equity(random_roll, bet.amount);
    if(payout.amount > 0) {
        action(permission_level{_self, N(active)},
               N(eosio.token),
               N(transfer),
               make_tuple(_self, bet.player, payout, equity_memo(bet)))
                .send();
    }
    */
}
void fairdicegame::addtoken(account_name contract, asset quantity)
{
    require_auth(_self);
    auto itr = _tokens.find(contract + quantity.symbol);
    if(itr == _tokens.end()) {
        _tokens.emplace(_self, [&](auto &r) {
            r.contract = contract;
            r.symbol = quantity.symbol;
            r.minAmout = quantity.amount;
        });
    }else{
        _tokens.modify(_tokens.get(contract + quantity.symbol), _self, [&](auto &r){
            r.minAmout = quantity.amount;
        });
    }
}
void fairdicegame::clear(string table, uint32_t numbers)
{
    require_auth(_self);
    uint32_t count = 0 ;
    string bets = "bets";
    string hash = "hash";
    string fundpool = "fundpool";
    string tokens = "tokens";
    if(table == bets){
        //empty bets table
        for(auto itr = _bets.begin(); itr != _bets.end() && count <= numbers;) {
            itr = _bets.erase(itr);
            count++;
        }
    }
    else if(table == hash){
        //empty hash table
        for(auto itr = _hash.begin(); itr != _hash.end() && count <= numbers;) {
            itr = _hash.erase(itr);
            count++;
        }
    }
    else if(table == fundpool){
        _fund_pool.remove();
    }
    else if(table == tokens){
        for(auto itr = _tokens.begin(); itr != _tokens.end() && count <= numbers;) {
            itr = _tokens.erase(itr);
            count++;
        }
    }
    else{
        eosio_assert(0, "wrong tables");
    }

}
void fairdicegame::init()
{
    require_auth(_self);
    //empty global table
    _global.remove();
    st_global global = _global.get_or_default();
    global.current_id = 1;
    global.eosperdice = EOSPERDICE;
    global.nexthalve = NEXTMINE;
    global.initStatu = 1;
    _global.set(global, _self);
}
