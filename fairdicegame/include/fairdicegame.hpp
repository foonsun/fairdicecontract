#include <algorithm>
#include <eosiolib/transaction.hpp>
#include "eosio.token.hpp"
#include "types.hpp"
#include <eosiolib/eosio.hpp>
#include "pradata.hpp"

class fairdicegame : public contract {
   public:
    fairdicegame(account_name self)
        : contract(self),
          _bets(_self, _self),
          _fund_pool(_self, _self),
          _hash(_self, _self),
          _users(_self, _self),
          _tokens(_self, _self),
          _global(_self, _self){}

    void transfer(const account_name& from, const account_name& to, const extended_asset& quantity, const string& memo);

    // @abi action
    void receipt(const st_bet& bet);

    // @abi action
    void reveal(const uint64_t& id, const checksum256& seed);

    // @abi action
    void result(const st_result& result);

    // @abi action
    void equity(const asset& quantity);

    // @abi action
    void addtoken(account_name contract, asset quantity);

    // @abi action
    void init();

    // @abi action
    void clear(string table, uint32_t numbers);

    void apply(account_name code, action_name action);

   private:
    tb_bets _bets;
    tb_fund_pool _fund_pool;
    tb_hash _hash;
    tb_global _global;
    tb_tokens _tokens;
    tb_users1 _users;
    void parse_memo(string memo,
                    /* uint8_t* roll_under, */
                    checksum256* seed_hash,
                    checksum160* user_seed_hash,
                    uint64_t* expiration,
                    account_name* referrer,
                    signature* sig) {
        // remove space
        memo.erase(std::remove_if(memo.begin(),
                                  memo.end(),
                                  [](unsigned char x) { return std::isspace(x); }),
                   memo.end());

        size_t sep_count = std::count(memo.begin(), memo.end(), '-');
        //eosio_assert(sep_count == 5, "invalid memo");
        eosio_assert(sep_count == 4, "invalid memo");

        size_t pos;
        string container;
        /*
        pos = sub2sep(memo, &container, '-', 0, true);
        eosio_assert(!container.empty(), "no roll under");
        *roll_under = stoi(container);
        */
        //pos = sub2sep(memo, &container, '-', ++pos, true);
        pos = sub2sep(memo, &container, '-', 0, true);
        eosio_assert(!container.empty(), "no seed hash");
        *seed_hash = hex_to_sha256(container);
        pos = sub2sep(memo, &container, '-', ++pos, true);
        eosio_assert(!container.empty(), "no user seed hash");
        *user_seed_hash = hex_to_sha1(container);
        pos = sub2sep(memo, &container, '-', ++pos, true);
        eosio_assert(!container.empty(), "no expiration");
        *expiration = stoull(container);
        pos = sub2sep(memo, &container, '-', ++pos, true);
        eosio_assert(!container.empty(), "no referrer");
        *referrer = string_to_name(container.c_str());
        container = memo.substr(++pos);
        eosio_assert(!container.empty(), "no signature");
        *sig = str_to_sig(container);
    }
    uint8_t compute_random_roll(const checksum256& seed1, const checksum160& seed2, uint8_t (&random_roll)[6]) {

        size_t hash = 0;
        hash_combine(hash, sha256_to_hex(seed1));
        hash_combine(hash, sha1_to_hex(seed2));
        random_roll[0] = hash % 6 + 1;
        hash_combine(hash, sha256_to_hex(seed1));
        hash_combine(hash, sha1_to_hex(seed2));
        random_roll[1] = hash % 6 + 1;
        hash_combine(hash, sha256_to_hex(seed1));
        hash_combine(hash, sha1_to_hex(seed2));
        random_roll[2] = hash % 6 + 1;
        hash_combine(hash, sha256_to_hex(seed1));
        hash_combine(hash, sha1_to_hex(seed2));
        random_roll[3] = hash % 6 + 1;
        hash_combine(hash, sha256_to_hex(seed1));
        hash_combine(hash, sha1_to_hex(seed2));
        random_roll[4] = hash % 6 + 1;
        hash_combine(hash, sha256_to_hex(seed1));
        hash_combine(hash, sha1_to_hex(seed2));
        random_roll[5] = hash % 6 + 1;
        return 0;
    }

    asset compute_referrer_reward(const st_bet& bet) { return bet.amount / 200; }

    uint64_t next_id() {
        //st_global global = _global.get_or_default(
        //    st_global{.current_id = _bets.available_primary_key()});
        st_global global = _global.get_or_default();
        global.current_id += 1;
        _global.set(global, _self);
        return global.current_id;
    }

    string referrer_memo(const st_bet& bet) {
        string memo = "bet id:";
        string id = uint64_string(bet.id);
        memo.append(id);
        memo.append(" player: ");
        string player = name{bet.player}.to_string();
        memo.append(player);
        memo.append(" referral reward! ");
        return memo;
    }

    string winner_memo(const st_bet& bet) {
        string memo = "bet id:";
        string id = uint64_string(bet.id);
        memo.append(id);
        memo.append(" player: ");
        string player = name{bet.player}.to_string();
        memo.append(player);
        memo.append(" congratulations! ");
        return memo;
    }

    string equity_memo(){
        string memo = " go on! ";
        return memo;
    }
    st_bet find_or_error(const uint64_t& id) {
        auto itr = _bets.find(id);
        eosio_assert(itr != _bets.end(), "bet not found");
        return *itr;
    }

    void assert_hash(const checksum256& seed_hash, const uint64_t& expiration) {
        const uint64_t _now = uint64_t(now())*1000;
        // check expiratin
        eosio_assert(expiration > _now, "seed hash expired");

        // check hash duplicate
        const uint64_t key = uint64_hash(seed_hash);
        auto itr = _hash.find(key);
        eosio_assert(itr == _hash.end(), "hash duplicate");

        // clean up
        auto index = _hash.get_index<N(by_expiration)>();
        auto upper_itr = index.upper_bound(_now);
        auto begin_itr = index.begin();
        auto count = 0;
        //eosio::print("upper_itr",upper_itr->expiration,"begin_itr",begin_itr->expiration);
        while ((begin_itr != upper_itr) && (count < 3)) {
            begin_itr = index.erase(begin_itr);
            count++;
        }

        // save hash
        _hash.emplace(_self, [&](st_hash& r) {
            r.hash = seed_hash;
            r.expiration = expiration;
        });
    }

    void assert_quantity(const extended_asset& quantity) {
        auto itr = _tokens.find(quantity.contract + quantity.symbol);
        eosio_assert(itr != _tokens.end(), "Non-existent token");
        eosio_assert(quantity.is_valid(), "quantity invalid");
        eosio_assert(quantity.amount >= itr->minAmout, "transfer quantity must be greater than minimum");
    }

    void assert_roll_under(/* const uint8_t& roll_under , */ const extended_asset& quantity) {
        /* eosio_assert(roll_under >= 2 && roll_under <= 96,
                     "roll under overflow, must be greater than 2 and less than 96");
        */
         eosio_assert(
            max_payout(/* roll_under, */ quantity) <= max_bonus(quantity),
            "offered overflow, expected earning is greater than the maximum bonus");
    }

    void save(const st_bet& bet) {
        _bets.emplace(_self, [&](st_bet& r) {
            r.id = bet.id;
            r.player = bet.player;
            r.referrer = bet.referrer;
            r.amount = bet.amount;
            /* r.roll_under = bet.roll_under; */
            r.seed_hash = bet.seed_hash;
            r.user_seed_hash = bet.user_seed_hash;
            r.created_at = bet.created_at;
        });
    }

    void remove(const st_bet& bet) { _bets.erase(bet); }

    void unlock(const asset& amount) {
        st_fund_pool pool = get_fund_pool();
        pool.locked -= amount;
        eosio_assert(pool.locked.amount >= 0, "fund unlock error");
        _fund_pool.set(pool, _self);
    }

    void lock(const asset& amount) {
        st_fund_pool pool = get_fund_pool();
        pool.locked += amount;
        _fund_pool.set(pool, _self);
    }

    asset compute_payout(/* const uint8_t& roll_under, */ const uint8_t (&random_roll)[6] , const extended_asset& offer) {
        uint8_t one_count = 0;
        uint8_t two_count = 0;
        uint8_t three_count = 0;
        uint8_t four_count = 0;
        uint8_t five_count = 0;
        uint8_t six_count = 0;
        for(int i = 0; i < sizeof(random_roll); i++) {
            uint8_t roll_number = random_roll[i];
            switch(roll_number){
                case 1:
                    one_count++;
                    break;
                case 2:
                    two_count++;
                    break;
                case 3:
                    three_count++;
                    break;
                case 4:
                    four_count++;
                    break;
                case 5:
                    five_count++;
                    break;
                case 6:
                    six_count++;
                    break;
                default:
                    eosio::print("roll number must be between 1 and 6\n");
            }
        }
// calculate result
        asset payout = asset(0, offer.symbol);
        if (six_count == 6 || five_count == 6 || three_count == 6 || two_count == 6) {
            payout.amount = offer.amount * GUANDENGQIANG;
        }
        else if(four_count == 6){
            payout.amount = offer.amount * LIUBEIHONG;
        }
        else if(four_count == 4 && one_count == 2){
            payout.amount = offer.amount * CHAJINHUA;
        }
        else if(one_count == 6){
            payout.amount = offer.amount * BIANDIJIN;
        }
        else if(four_count == 5){
            payout.amount = offer.amount * WUHONG;
        }
        else if(six_count == 5 || five_count == 5 || three_count == 5 || two_count == 5 || one_count == 5){
            payout.amount = offer.amount * WUZIDENGKE;
        }
        else if(four_count == 4){
            payout.amount = offer.amount * SIDIANHONG;
        }
        else if(one_count == 1 && two_count == 1 && three_count == 1 && four_count == 1 && five_count == 1 && six_count == 1){
            payout.amount = offer.amount * DUITANG;
        }
        else if(four_count == 3){
            payout.amount = offer.amount * SANHONG;
        }
        else if(three_count == 4 || two_count == 4 || one_count == 4 || five_count == 4 || six_count == 4){
            payout.amount = offer.amount * SIJIN;
        }
        else if(four_count == 2){
            payout.amount = offer.amount * ERJU;
        }
        else if(four_count == 1){
            payout.amount = offer.amount * YIXIU;
        }
        else{
            payout.amount = 0;
        }
        return min(payout, max_bonus(offer));
    }

    asset max_payout(/* const uint8_t& roll_under, */ const extended_asset& offer) {
//      const double ODDS = 98.0 / ((double)roll_under - 1.0);
        //max pay amount can be awarded.
        return asset(MAX_RATIO * offer.amount, offer.symbol);
    }

    asset max_bonus(const extended_asset &quantity) { return available_balance(quantity) / 2; }

    asset available_balance(const extended_asset &quantity) {
        auto token = eosio::token(quantity.contract);
        const asset balance =
                token.get_balance(_self, symbol_type(quantity.symbol).name());
        asset available;
        if (iseostoken(quantity))
        {
            const asset locked = get_fund_pool().locked;
            available = balance - locked;
        }
        else
        {
            available = balance;
        }
        eosio_assert(available.amount >= 0, "fund pool overdraw");
        return available;
    }

    st_fund_pool get_fund_pool() {
        st_fund_pool fund_pool{.locked = asset(0, EOS_SYMBOL)};
        return _fund_pool.get_or_create(_self, fund_pool);
    }

    void assert_signature(/* const uint8_t& roll_under, */
                          const checksum256& seed_hash,
                          const uint64_t& expiration,
                          const account_name& referrer,
                          const signature& sig) {
        /* string data = uint64_string(roll_under);
        data += "-"; */
        string data = sha256_to_hex(seed_hash);
        data += "-";
        data += uint64_string(expiration);
        data += "-";
        data += name{referrer}.to_string();
        checksum256 digest;
        const char* data_cstr = data.c_str();
        sha256(data_cstr, strlen(data_cstr), &digest);
        public_key key = str_to_pub(PUB_KEY, false);
        assert_recover_key(&digest,
                           (char*)&sig.data,
                           sizeof(sig.data),
                           (char*)&key.data,
                           sizeof(key.data));
    }

    void assert_seed(const checksum256& seed, const checksum256& hash) {
        string seed_str = sha256_to_hex(seed);
        assert_sha256(seed_str.c_str(),
                      strlen(seed_str.c_str()),
                      (const checksum256*)&hash);
    }

    template <typename... Args>
    void send_defer_action(Args&&... args) {
        transaction trx;
        trx.actions.emplace_back(std::forward<Args>(args)...);
        trx.delay_sec = 1;
        trx.send(next_id(), _self, false);
    }

    void check_account1(account_name from)
    {
        auto db = prochain::rating_index(N(rating.pra), N(rating.pra));
        auto me = db.find(from);
        if (me != db.end())
        {
            auto account_type = me->account_type;
            eosio_assert(account_type == 0, "Human only");
        }
    }

    bool iseostoken(const extended_asset &quantity)
    {
        if ((quantity.contract == N(eosio.token)) && (quantity.symbol == EOS_SYMBOL))
        {
            return true;
        }
        return false;
    }

    void iplay(account_name from, asset quantity)
    {
        tb_users1 _users1(_self, from);
        auto v = _users1.get_or_create(_self, st_user1{});
        v.amount += quantity;
        v.count += 1;
        _users1.set(v, _self);
    }
};

struct st_transfer
{
    account_name from;
    account_name to;
    asset quantity;
    string memo;
};


void fairdicegame::apply(account_name code, action_name action)
{
    auto &thiscontract = *this;

    if (action == N(transfer))
    {
        auto transfer_data = unpack_action_data<st_transfer>();
        transfer(transfer_data.from, transfer_data.to, extended_asset(transfer_data.quantity, code), transfer_data.memo);
        return;
    }

    if (code != _self)
        return;
    switch (action)
    {
        EOSIO_API(fairdicegame, (receipt)(reveal)(result)(equity)(init)(addtoken)(clear));
    };
}

extern "C"
{
    [[noreturn]] void apply(uint64_t receiver, uint64_t code, uint64_t action) {
        fairdicegame p(receiver);
        p.apply(code, action);
        eosio_exit(0);
    }
}
