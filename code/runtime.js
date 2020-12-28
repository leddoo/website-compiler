function is_dom_element(e) {
    return e instanceof Element || e instanceof HTMLDocument;
}

class Tree_Node {

    // NOTE(llw): "dom" is to be put into the browser dom by the caller.
    constructor(parent, dom, name) {
        console.assert(is_dom_element(dom));

        this.tn_parent = parent || null;
        this.tn_dom    = dom;
        this.tn_name   = name;

        console.assert(!('tree_node' in dom));
        dom.tree_node = this;

        if(parent) {
            console.assert(
                !(name in this.tn_parent),
                "Error: " + name + " already in " + this.tn_parent.tn_dom.id
            );

            this.tn_dom.id = this.tn_parent.tn_dom.id + "-" + name;
            this.tn_parent[name] = this;
        }
    }


    tn_remove() {
        console.assert(!this.tn_is_list_item);

        // NOTE(llw): Remove from dom.
        this.tn_dom.parentNode.removeChild(this.tn_dom);

        // NOTE(llw): Remove from tree.
        delete this.tn_parent[this.tn_name];
    }


    tn_add_wrapper(name) {
        console.assert(typeof name === "string");

        let div = document.createElement("div");
        div.id = this.tn_dom.id + "-" + name;

        let node = new Tree_Node(this, div, name);
        this.tn_dom.append(div);

        return node;
    }


    tn_for(callback) {
        for(const entry in this) {
            if(    this[entry] instanceof Tree_Node
                && !entry.startsWith("tn_")
            ) {
                callback(this[entry]);
            }
        }
    }


    tn_clear() {
        let entries = [];
        this.tn_for(function(entry) { entries.push(entry); });
        for(const entry of entries) {
            entry.tn_remove();
        }

        this.tn_dom.innerHTML = "";
    }



    tn_listify(make_entry, min, max) {
        console.assert(!this.tn_is_list);

        console.assert(typeof make_entry === "function");
        console.assert(typeof min === "number");
        console.assert(typeof max === "number");

        this.tn_list_min   = min;
        this.tn_list_max   = max;
        this.tn_list_count = 0;

        this.tn_list_insert_new = Tree_Node.prototype._tn_list_insert_new;
        this._tn_list_make_entry = make_entry;
        this.tn_dom.classList.add("tn_list");

        this.tn_is_list = true;

        // NOTE(llw): Itemify children.
        for(let i = 0; true; i += 1) {
            let name = "tn_list_item_" + i;
            if(!(name in this)) {
                break;
            }

            let child = this[name];
            console.assert(child instanceof Tree_Node);

            child._tn_list_itemify(i);

            this.tn_list_count += 1;
        }

        this._tn_list_next_id = this.tn_list_count;

        console.assert(min <= this.tn_list_count);
        console.assert(max >= this.tn_list_count);
    }

    _tn_list_itemify(index) {
        this.tn_list_item_index = index;
        this.tn_parent[index] = this;

        this.tn_dom.classList.add("tn_list_item");
        this.tn_dom.classList.add("tn_list_item_" + index);

        this.tn_remove = Tree_Node.prototype._tn_list_remove;
        this.tn_is_list_item = true;
    }

    _tn_list_set_index(new_index) {
        let old_index = this.tn_list_item_index;
        delete (this.tn_parent)[old_index];
        this.tn_parent[new_index] = this;
        this.tn_list_item_index = new_index;

        this.tn_dom.classList.remove("tn_list_item_" + old_index);
        this.tn_dom.classList.add   ("tn_list_item_" + new_index);
    }

    _tn_list_insert_new(at) {
        console.assert(this.tn_is_list);
        console.assert(this.tn_list_count < this.tn_list_max);

        // NOTE(llw): Default is append.
        if(at === undefined) {
            at = this.tn_list_count;
        }

        console.assert(typeof at === "number");
        console.assert(at >= 0 && at <= this.tn_list_count);

        // NOTE(llw): Move others up.
        for(let i = this.tn_list_count - 1; i >= at; i -= 1) {
            this[i]._tn_list_set_index(i + 1);
        }

        // NOTE(llw): Instantiate new element.
        let id = "tn_list_item_" + this._tn_list_next_id;
        this._tn_list_next_id += 1;
        this.tn_list_count += 1;

        let wrapper = this.tn_add_wrapper(id);
        wrapper._tn_list_itemify(at);
        this._tn_list_make_entry(wrapper);

        // NOTE(llw): Move to right place in dom.
        if(at + 1 < this.tn_list_count) {
            this.tn_dom.insertBefore(wrapper.tn_dom, this[at + 1].tn_dom);
        }

        return this[at];
    }

    _tn_list_remove() {
        console.assert(this.tn_is_list_item);
        console.assert(this.tn_parent.tn_is_list);
        console.assert(this.tn_parent.tn_list_count > this.tn_parent.tn_list_min);

        let list = this.tn_parent;
        let at = this.tn_list_item_index;

        // NOTE(llw): Remove this entry.
        this.tn_is_list_item = false;
        Tree_Node.prototype.tn_remove.call(this);
        delete list[at];

        // NOTE(llw): Move others down.
        for(let i = at + 1; i < list.tn_list_count; i += 1) {
            list[i]._tn_list_set_index(i - 1);
        }
        list.tn_list_count -= 1;
    }
}

