//
//  LruCache.h LRU缓存类，用于在内存中缓存经常出现的数据
//
//
//  Created by zhouxuguang on 19/4/19.
//  Copyright © 2019年 zhouxuguang. All rights reserved.
//

#ifndef BAASELIB_LRU_CACHE_INCLUDE_H
#define BAASELIB_LRU_CACHE_INCLUDE_H

#include <unordered_set>
#include "PreCompile.h"

NS_BASELIB_BEGIN

    /**
     * GenerationCache callback used when an item is removed
     */
    template<typename EntryKey, typename EntryValue>
    class OnEntryRemoved 
	{
    public:
        virtual ~OnEntryRemoved() { };
        virtual void operator()(EntryKey& key, EntryValue& value) = 0;
    }; // class OnEntryRemoved

    //模板的第一个参数是key的类型，第二个参数是值的类型，第三个参数是key的哈希计算方法
    template <typename TKey, typename TValue, typename THashFunc = std::hash<TKey> >
    class LruCache
	{
    public:
        explicit LruCache(unsigned int maxCapacity);
        
        ~LruCache();
        
        enum Capacity 
		{
            kUnlimitedCapacity,
        };
        
        //设置移除监听器
        void SetOnEntryRemovedListener(OnEntryRemoved<TKey, TValue>* listener);
        
        //当前缓存项的个数
        size_t GetSize() const;
        
        //设置最大容量
        void SetMaxCapacity(unsigned int maxCapacity);
        
        //根据键取得值
        const TValue& Get(const TKey& key) const;
        
        //将键值对放到缓存中
        bool Put(const TKey& key, const TValue& value);
        
        //查询key的值是否存在
        bool Contains(const TKey &key) const;
        
        //移除键为key的项
        bool Remove(const TKey& key);
        
        //清除全部缓存项
        void Clear();
        
    private:
        LruCache(const LruCache& that);  // disallow copy constructor
        
        //移除最早的数据
        bool RemoveOldest();
        
        //挑选最旧的数据
        const TValue& PeekOldestValue() const;
        
        struct Entry 
		{
            TKey key;
            TValue value;
            Entry* parent;
            Entry* child;
            
            Entry(TKey key_, TValue value_) : key(key_), value(value_), parent(NULL), child(NULL) 
			{
            }
            const TKey& getKey() const { return key; }
        };
        
        struct HashForEntry //: public std::unary_function<Entry*, size_t>
        {
            size_t operator() (const Entry* entry) const
            {
                return THashFunc()(entry->key);
            };
        };
        
        struct EqualityForHashedEntries //: public std::unary_function<Entry*, bool>
		{
            bool operator() (const Entry* lhs, const Entry* rhs) const 
			{
                return lhs->key == rhs->key;
            };
        };
        
        typedef std::unordered_set<Entry*, HashForEntry, EqualityForHashedEntries> LruCacheSet;
        
        void attachToCache(Entry& entry) const;
        
        void detachFromCache(Entry& entry) const;
        
        typename LruCacheSet::iterator findByKey(const TKey& key) const
		{
            Entry entryForSearch(key, mNullValue);
            typename LruCacheSet::iterator result = mSet->find(&entryForSearch);
            return result;
        }
        
        LruCacheSet* mSet;
        OnEntryRemoved<TKey, TValue>* mListener;
        mutable Entry* mOldest;
        mutable Entry* mYoungest;
        unsigned int mMaxCapacity;
        TValue mNullValue;
        
    public:
        // To be used like:
        // while (it.next())
        // {
        //   it.value(); it.key();
        // }
        class Iterator 
		{
        public:
            Iterator(const LruCache<TKey, TValue, THashFunc>& cache):
            mCache(cache), mIterator(mCache.mSet->begin()), mBeginReturned(false) 
			{
            }
            
            bool next() 
			{
                if (mIterator == mCache.mSet->end()) 
				{
                    return false;
                }
                if (!mBeginReturned) 
				{
                    // mIterator has been initialized to the beginning and
                    // hasn't been returned. Do not advance:
                    mBeginReturned = true;
                } 
				else 
				{
                    std::advance(mIterator, 1);
                }
                return mIterator != mCache.mSet->end();
            }
            
            const TValue& value() const 
			{
                return (*mIterator)->value;
            }
            
            const TKey& key() const 
			{
                return (*mIterator)->key;
            }
        private:
            const LruCache<TKey, TValue, THashFunc>& mCache;
            typename LruCacheSet::iterator mIterator;
            bool mBeginReturned;
        };
    };
    
    // Implementation is here, because it's fully templated
    template <typename TKey, typename TValue, typename THashFunc>
    LruCache<TKey, TValue, THashFunc>::LruCache(unsigned int maxCapacity)
    : mSet(new(std::nothrow) LruCacheSet())
    , mListener(NULL)
    , mOldest(NULL)
    , mYoungest(NULL)
    , mMaxCapacity(maxCapacity)
    , mNullValue(TValue())
	{
        mSet->max_load_factor(1.0);
    };
    
    template <typename TKey, typename TValue, typename THashFunc>
    LruCache<TKey, TValue, THashFunc>::~LruCache()
	{
        // Need to delete created entries.
        Clear();
        delete mSet;
        mSet = NULL;
    };
    
    template<typename TKey, typename TValue, typename THashFunc>
    void LruCache<TKey, TValue, THashFunc>::SetOnEntryRemovedListener(OnEntryRemoved<TKey, TValue>* listener)
	{
        mListener = listener;
    }
    
    template <typename TKey, typename TValue, typename THashFunc>
    size_t LruCache<TKey, TValue, THashFunc>::GetSize() const
	{
        return mSet->size();
    }
    
    template <typename TKey, typename TValue, typename THashFunc>
    void LruCache<TKey,TValue, THashFunc>::SetMaxCapacity(unsigned int maxCapacity)
    {
        this->mMaxCapacity = maxCapacity;
    }
    
    template <typename TKey, typename TValue, typename THashFunc>
    bool LruCache<TKey,TValue, THashFunc>::Contains(const TKey &key) const
    {
        typename LruCacheSet::const_iterator find_result = findByKey(key);
        if (find_result != mSet->end()) 
		{
            return true;
        }
        return false;
    }
    
    template <typename TKey, typename TValue, typename THashFunc>
    const TValue& LruCache<TKey, TValue, THashFunc>::Get(const TKey& key) const
	{
        typename LruCacheSet::const_iterator find_result = findByKey(key);
        if (find_result == mSet->end()) 
		{
            return mNullValue;
        }
        Entry *entry = *find_result;
        detachFromCache(*entry);
        attachToCache(*entry);
        return entry->value;
    }
    
    template <typename TKey, typename TValue, typename THashFunc>
    bool LruCache<TKey, TValue, THashFunc>::Put(const TKey& key, const TValue& value)
	{
        if (mMaxCapacity != kUnlimitedCapacity && GetSize() >= mMaxCapacity) 
		{
            RemoveOldest();
        }
        
        if (findByKey(key) != mSet->end()) 
		{
            return false;
        }
        
        Entry* newEntry = new(std::nothrow) Entry(key, value);
        mSet->insert(newEntry);
        attachToCache(*newEntry);
        return true;
    }
    
    template <typename TKey, typename TValue, typename THashFunc>
    bool LruCache<TKey, TValue, THashFunc>::Remove(const TKey& key)
	{
        typename LruCacheSet::const_iterator find_result = findByKey(key);
        if (find_result == mSet->end()) 
		{
            return false;
        }
        Entry* entry = *find_result;
        mSet->erase(entry);
        if (mListener) 
		{
            (*mListener)(entry->key, entry->value);
        }
        detachFromCache(*entry);
        delete entry;
        return true;
    }
    
    template <typename TKey, typename TValue, typename THashFunc>
    bool LruCache<TKey, TValue, THashFunc>::RemoveOldest()
	{
        if (mOldest != NULL) 
		{
            return Remove(mOldest->key);
            // TODO: should probably abort if false
        }
        return false;
    }
    
    template <typename TKey, typename TValue, typename THashFunc>
    const TValue& LruCache<TKey, TValue, THashFunc>::PeekOldestValue() const
	{
        if (mOldest) 
		{
            return mOldest->value;
        }
        return mNullValue;
    }
    
    template <typename TKey, typename TValue, typename THashFunc>
    void LruCache<TKey, TValue, THashFunc>::Clear()
	{
        if (mListener) 
		{
            for (Entry* p = mOldest; p != NULL; p = p->child) 
			{
                (*mListener)(p->key, p->value);
            }
        }
        mYoungest = NULL;
        mOldest = NULL;
        
        if (NULL == mSet)
        {
            return;
        }
        
        typename LruCacheSet::iterator iter = mSet->begin();
        for (; iter != mSet->end(); ++ iter)
        {
            Entry* entry = *iter;
            delete entry;
        }
        
        mSet->clear();
    }
    
    template <typename TKey, typename TValue, typename THashFunc>
    void LruCache<TKey, TValue, THashFunc>::attachToCache(Entry& entry) const
	{
        if (mYoungest == NULL) 
		{
            mYoungest = mOldest = &entry;
        } 
		else 
		{
            entry.parent = mYoungest;
            mYoungest->child = &entry;
            mYoungest = &entry;
        }
    }
    
    template <typename TKey, typename TValue, typename THashFunc>
    void LruCache<TKey, TValue, THashFunc>::detachFromCache(Entry& entry) const
	{
        if (entry.parent != NULL) 
		{
            entry.parent->child = entry.child;
        } 
		else 
		{
            mOldest = entry.child;
        }

        if (entry.child != NULL) 
		{
            entry.child->parent = entry.parent;
        } 
		else 
		{
            mYoungest = entry.parent;
        }
        
        entry.parent = NULL;
        entry.child = NULL;
    }

NS_BASELIB_END

#endif /* BAASELIB_LRU_CACHE_INCLUDE_H */
