#ifndef __WLIST_H
#define __WLIST_H


namespace WDB {

	typedef unsigned char u1b;

	template<class T> class List_entry {
		public:
			T *next;
			T *pre;
			List_entry() : pre(0), next(0) {}
			bool isnext() { return (next ? true : false ); }
			T *getnext() { return (isnext() ? next : 0); } 
	};

	template<class T> class List_head {
		public:
			T *first;
			T *last;
			List_head() : first(0), last(0) {}
	};

	class Shlist_entry {
			int pre;
			int next;
		public:

			Shlist_entry() : pre(0), next(0) {}

			void init() { pre = next = 0;}

			Shlist_entry *get_next() {
				if (next != 0){
					return (Shlist_entry *)((u1b *)this + next);
				}
				return NULL;
			}

			Shlist_entry *get_pre() {
				if (pre != 0) {
					return (Shlist_entry *)((u1b *)this + pre);
				}	
				return NULL;
			}

			void insert_after(Shlist_entry *entry) {
				if (next) {
					Shlist_entry *entry_next = get_next();
					entry->next = (u1b *)entry_next - (u1b *)entry;
					entry_next->pre = -entry->next;
				}
				next = (u1b *)(entry) - (u1b *)this;
				entry->pre = -next;
			}

			void insert_before(Shlist_entry *entry) {
				Shlist_entry *entry_pre= get_pre();
				entry_pre->next = (u1b *)this - (u1b *)entry_pre;
				entry->pre = -entry_pre->next;
				entry->next = (u1b *)this - (u1b *)entry;
				pre = -entry->next;
			}

			void remove() {
				Shlist_entry *entry_pre = get_pre();
				Shlist_entry *entry_next = get_next();
				entry_pre->next = next - pre;
				entry_next->pre = pre - next;
			}

	};

	class Shlist_head {
			int first;
			int last;
		public:
			Shlist_head() : first(0), last(0) {}

			void init() { first = last = 0;}

			Shlist_entry *get_first() {
				if (first == 0)
					return NULL;
				else
					return (Shlist_entry *)((u1b *)this + first);
			}

			Shlist_entry *get_last() {
				if (last == 0)
					return NULL;
				else
					return (Shlist_entry *)((u1b *)this + last);
			}

			bool is_empty() { return (first == 0 || last == 0) ? true : false;}

			void insert_head(Shlist_entry *entry) {
				Shlist_entry *fentry = get_first();
				if (fentry) {
					fentry->pre = (u1b *)entry - (u1b *)fentry;
					entry->next = -fentry->pre;
				}
				first = (u1b *)entry - (u1b *)this;
				if (last == 0)
					last = first;
			}

			void insert_tail(Shlist_entry *entry) {
				Shlist_entry *lentry = get_last();
				if (lentry) {
					lentry->next = (u1b *)entry - (u1b *)lentry;
					entry->pre = -lentry->next;
				}
				last = (u1b *)entry - (u1b *)this;
				if (first == 0)
					first = last;
			}

			void remove_first() {
				Shlist_entry *fentry = get_first();
				if (!fentry) return;
				if (fentry->next == 0) {
					first = 0;
					last = 0;
					return;
				}
				Shlist_entry *nfentry = (Shlist_entry *)((u1b *)fentry + fentry->next);
				first = (u1b *)nfentry - (u1b *)this;
				nfentry->pre = -first;
			}

			void remove_last() {
				if (first == last) {
					remove_first();
					return;
				}
				Shlist_entry *lentry = get_last();
				Shlist_entry *nlentry = (Shlist_entry *)((u1b *)lentry + lentry->pre);
				nlentry->next = 0;
				last = (u1b *)nlentry - (u1b *)this;
			}
	};

	static inline void shremove(Shlist_head *he, Shlist_entry *e) {
		if (he->get_first() == e) {
			he->remove_first();
			return;
		}
		if (he->get_last() == e) {
			he->remove_last();
			return;
		}
		e->remove();
	}
};
#endif


