#pragma once
#include <typeindex>

#include "FlexibleVector.h"
#include "ThreadPool.h"

using ObjectKey = size_t;
using HashKey   = std::type_index;

template <typename Ty>
class EntitiesQuery {
private:
    FlexibleVector<>* ptr;
    ThreadPool* threads;

public:
    void build(FlexibleVector<>* ptr, ThreadPool* threads) {
        this->ptr     = ptr;
        this->threads = threads;
    }

    Ty* at(const size_t idx) {
        return this->ptr->at<Ty>(idx);
    }
    const size_t size() const { return this->ptr->size(); }

    EntitiesQuery& for_each(const std::function<void(Ty&)>& fun) {
        for (size_t i = 0; i < this->ptr->size(); i++) {
            if (i + 1 < this->ptr->size()) {
                _mm_prefetch(reinterpret_cast<const char*>(this->ptr->at<Ty>(i + 1)), _MM_HINT_T0);
            }
            fun(*this->ptr->at<Ty>(i));
        }

        return *this;
    }
    EntitiesQuery& for_indexed(const std::function<void(int, Ty&)>& fun) {
        for (size_t i = 0; i < this->ptr->size(); i++) {
            if (i + 1 < this->ptr->size()) {
                _mm_prefetch(reinterpret_cast<const char*>(this->ptr->at<Ty>(i + 1)), _MM_HINT_T0);
            }
            fun(i, *this->ptr->at<Ty>(i));
        }

        return *this;
    }

    EntitiesQuery& for_each_multithreaded(const std::function<void(Ty&)>& fun, const bool wait) {
        auto work = [this, fun](int i) {
            fun(*this->ptr->at<Ty>(i));
            };

        ThreadGroup* group = threads->scheduleWorkIndexed(this->ptr->size(), work);
        if (wait) group->join();
        delete group;

        return *this;
    }
    EntitiesQuery& for_indexed_multithreaded(const std::function<void(int, Ty&)>& fun, bool wait) {
        auto work = [this, fun](int i) {
            fun(i, *this->ptr->at<Ty>(i));
            };

        ThreadGroup* group = threads->scheduleWorkIndexed(this->ptr->size(), work);
        if (wait) group->join();
        delete group;

        return *this;
    }
};

class ObjectsManager {
private:
    std::unordered_map<HashKey, FlexibleVector<>> storage;

    ThreadPool threads;

public:
    void build(const size_t initialSize, const size_t threadsAmount) {
        this->storage.reserve(initialSize);

        this->threads.build(threadsAmount);
    }

    template <typename Ty, typename = std::enable_if_t<std::is_copy_constructible_v<Ty>>>
    void createEntity(const Ty& entity) {
        auto it = this->storage.find(typeid(Ty));
        if (it != this->storage.end()) {
            it->second.push(entity);
            return;
        }

        FlexibleVector group;
        group.build<Ty>();
        group.push(entity);

        this->storage.insert({ typeid(Ty), std::move(group) });
    }
    template <typename Ty>
    void createEntity(Ty&& entity) {
        auto it = this->storage.find(typeid(Ty));
        if (it != this->storage.end()) {
            it->second.push(std::move(entity));
            return;
        }

        FlexibleVector group;
        group.build<Ty>();
        group.push(std::move(entity));

        this->storage.insert({ typeid(Ty), std::move(group) });
    }

    template <typename Ty>
    EntitiesQuery<Ty> get() {
        EntitiesQuery<Ty> query;

        auto it = this->storage.find(typeid(Ty));
        if (it == this->storage.end()) {
            query.build(nullptr, nullptr);
            return query;
        }

        query.build(&it->second, &this->threads);
        return query;
    }
};