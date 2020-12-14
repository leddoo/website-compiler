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
            this.tn_set_name(name);
        }
    }


    tn_remove() {
        console.assert(
               typeof this.tn_name === "string"
            || typeof this.tn_name === "number" && this.tn_parent.tn_is_list
        );

        // NOTE(llw): Remove from dom.
        this.tn_dom.parentNode.removeChild(this.tn_dom);

        // NOTE(llw): Remove from tree.
        delete this.tn_parent[this.tn_name];
    }


    tn_add_wrapper(name) {
        console.assert(
               typeof name === "string"
            || typeof name === "number" && this.tn_is_list
        );

        let div = document.createElement("div");
        div.id = this.tn_dom.id + "-" + name;

        let node = new Tree_Node(this, div, name);
        this.tn_dom.append(div);

        return node;
    }


    tn_set_name(name) {
        console.assert(
               typeof name === "string"
            || typeof name === "number" && this.tn_parent.tn_is_list
        );

        console.assert(
               !(name in this.tn_parent)
            || this.tn_parent[name] === this,
            "Error: " + name + " already in " + this.tn_parent.tn_dom.id
        );

        delete this.tn_parent[this.tn_name];
        this.tn_name = name;
        this.tn_dom.id = this.tn_parent.tn_dom.id + "-" + name;
        this.tn_parent[name] = this;
    }


    tn_clear() {
        for(const entry in this) {
            if(this[entry] instanceof Tree_Node) {
                delete this[entry];
            }
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

        this.tn_is_list = true;

        // NOTE(llw): Update children's tn_remove.
        for(let i = 0; i in this; i += 1) {
            console.assert(this[i] instanceof Tree_Node);

            this[i].tn_remove = Tree_Node.prototype._tn_list_remove;
            this[i].tn_name = i;

            this.tn_list_count += 1;
        }

        console.assert(min <= this.tn_list_count);
        console.assert(max >= this.tn_list_count);
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
            this[i].tn_set_name(i + 1);
        }
        this.tn_list_count += 1;

        // NOTE(llw): Instantiate new element.
        let wrapper = this.tn_add_wrapper(at);
        wrapper.tn_remove = Tree_Node.prototype._tn_list_remove;
        this._tn_list_make_entry(wrapper);

        // NOTE(llw): Move to right place in dom.
        if(at + 1 < this.tn_list_count) {
            this.tn_dom.insertBefore(wrapper.tn_dom, this[at + 1].tn_dom);
        }
    }

    _tn_list_remove() {
        console.assert(this.tn_parent.tn_is_list);
        console.assert(this.tn_parent.tn_list_count > this.tn_parent.tn_list_min);
        console.assert(typeof this.tn_name === "number");

        let list = this.tn_parent;
        let at = this.tn_name;

        // NOTE(llw): Remove this entry.
        Tree_Node.prototype.tn_remove.call(this);

        // NOTE(llw): Move others down.
        for(let i = at + 1; i < list.tn_list_count; i += 1) {
            list[i].tn_set_name(i - 1);
        }
        list.tn_list_count -= 1;
    }
}

