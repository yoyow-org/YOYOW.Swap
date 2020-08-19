#include <graphenelib/contract.hpp>
#include <graphenelib/dispatcher.hpp>
#include <graphenelib/multi_index.hpp>
#include <graphenelib/print.hpp>
#include <algorithm> 

using namespace graphene;

#define SWAP_STATUS_TABLE_ROW_ID 0
uint64_t sqrt(uint64_t y)
{
	uint64_t z;
	if (y > 3) {
        z = y;
        uint64_t x = y / 2 + 1;
        while (x < z) {
            z = x;
            x = (y / x + x) / 2;
        }
    } else if (y != 0) {
        z = 1;
    }
	return z;
}

class swap : public contract
{
  public:
    swap(uint64_t id)
        : contract(id),
        _status(_self,_self)
    {
    }

	//@abi action
	void initialize()
	{
		graphene_assert(get_trx_sender() == _self, "only owner can initialize contract");
		graphene_assert(_status.find(SWAP_STATUS_TABLE_ROW_ID) == _status.end(), "already initialized!");
		_status.emplace(0, [&](auto &o) {
			o.id = SWAP_STATUS_TABLE_ROW_ID;
            o.feeTo = 0;
            o.feeToSetter = 0;
			o.paircount = 0;
        });	
	}

	//@abi action    
    void createpair(const uint64_t& tokenA, const uint64_t& tokenB)
    {
		graphene_assert(tokenA != tokenB, "same token");
		uint64_t token0,token1;
		if(tokenA < tokenB)
		{
			token0 = tokenA;
			token1 = tokenB; 
		}
		else
		{
			token0 = tokenB; 
			token1 = tokenA;
		}

		swap_pair_index pairs(_self,token0);
		graphene_assert(pairs.find(token1) == pairs.end(),"pair exists");

		pairs.emplace(0, [&](auto &o) {
			o.factory = get_trx_sender();
            o.token0 = token0;
            o.token1 = token1;
            o.reserve0 = 0;
			o.reserve1 = 0;
			o.blockTimestampLast = 0;
			o.price0CumulativeLast = 0;
			o.price1CumulativeLast = 0;
			o.kLast = 0;
        });		
	}
	
    //@abi action
    void setfeeto(const uint64_t& feeto)
    {
		auto itr = _status.find(SWAP_STATUS_TABLE_ROW_ID);
		graphene_assert(itr != _status.end(), "contract not initialized!");


		_status.modify(itr, 0, [&](auto &o) {
            o.feeTo += feeto;
        });
    }
    
	//@abi action
    void setfeetosetter(const uint64_t& feetosetter)
	{
		auto itr = _status.find(SWAP_STATUS_TABLE_ROW_ID);
		graphene_assert(itr != _status.end(), "contract not initialized!");


		_status.modify(itr, 0, [&](auto &o) {
            o.feeToSetter += feetosetter;
        });
	}

  private:
    //@abi table
    struct swap_pair {
        uint64_t factory;
		uint64_t token0;
		uint64_t token1;
        uint64_t reserve0;
		uint64_t reserve1;
		uint32_t blockTimestampLast;

		uint64_t price0CumulativeLast;
		uint64_t price1CumulativeLast;

		uint64_t kLast;
		

        uint64_t primary_key() const { return token1; }

		uint64_t mint(uint64_t to);

		pair<uint64_t,uint64_t> burn(uint64_t to);

		void swap(uint64_t amount0Out, uint64_t amount1Out, uint64_t to, std::vector<unsigned char> data);

      
        GRAPHENE_SERIALIZE(swap_pair, (factory)(token0)(token1)(reserve0)(reserve1)(blockTimestampLast)(price0CumulativeLast)(price1CumulativeLast)(kLast))
    };

    typedef multi_index<N(swappair), swap_pair>  swap_pair_index;

	const swap_pair& get_pair(uint64_t token0,uint64_t token1)
	{
		if(token0 > token1)
			std::swap(token0,token1);
		
		swap_pair_index pairs(_self,token0);
		auto itr = pairs.find(token1);
		graphene_assert(itr != pairs.end(),"pair not exists");
		return *itr;
	}

	
    //@abi table
    struct swap_status {
    	uint64_t id;
        uint64_t feeTo;
		uint64_t feeToSetter;
		uint32_t paircount;

        uint64_t primary_key() const { return id; }

      
        GRAPHENE_SERIALIZE(swap_status, (id)(feeTo)(feeToSetter)(paircount))
    };

    typedef multi_index<N(swapstatus), swap_status>  swap_status_index;

	swap_status_index _status;

    
};

GRAPHENE_ABI(swap, (createpair)(setfeeto)(setfeetosetter))
