function is_dom_element(e) {
    return e instanceof Element || e instanceof HTMLDocument;
}

class Tree_Node {

    // NOTE(llw): "dom" is to be put into the browser dom by the caller.
    constructor(parent, dom, name) {
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
            || this.tn_remove === Tree_Node.prototype._tn_list_remove
        );

        // NOTE(llw): Remove from dom.
        this.tn_dom.parentNode.removeChild(this.tn_dom);

        // NOTE(llw): Remove from tree.
        delete this.tn_parent[this.tn_name];
    }


    tn_add_wrapper(name) {
        console.assert(typeof name === "string" || typeof name === "number");

        let div = document.createElement("div");
        div.id = this.tn_dom.id + "-" + name;

        let node = new Tree_Node(this, div, name);
        this.tn_dom.append(div);

        return node;
    }


    tn_set_name(name) {
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


    tn_list_insert_new(at) {
        console.assert("tn_list_count" in this);

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
        this._tn_list_make_new(wrapper);

        // NOTE(llw): Move to right place in dom.
        if(at + 1 < this.tn_list_count) {
            this.tn_dom.insertBefore(wrapper.tn_dom, this[at + 1].tn_dom);
        }
    }

    _tn_list_remove() {
        console.assert("tn_list_count" in this.tn_parent);
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

