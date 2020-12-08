function is_dom_element(e) {
    return e instanceof Element || e instanceof HTMLDocument;
}

// NOTE(llw): Result is added to the tree. Dom is not added to the dom.
class Tree_Node {
    constructor(parent, dom, name) {
        this.tn_parent = parent || null;
        this.tn_dom    = dom;
        this.tn_name   = name;

        console.assert(!('tree_node' in dom));
        dom.tree_node = this;

        if(parent) {
            console.assert(!(name in parent), name + " already in " + parent.tn_name);
            parent[name] = this;
        }
    }


    tn_remove() {
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
}

