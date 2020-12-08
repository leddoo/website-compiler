function is_dom_element(e) {
    return e instanceof Element || e instanceof HTMLDocument;
}

// NOTE(llw): Result is added to the tree. Dom is not added to the dom.
class Tree_Node {
    constructor(parent, dom, name, type) {
        this.Parent = parent || null;
        this.Dom    = dom;
        this.Name   = name;
        this.Type   = type || null;

        console.assert(!('tree_node' in dom));
        dom.tree_node = this;

        if(parent) {
            console.assert(!(name in parent), name + " already in " + parent.Name);
            parent[name] = this;
        }
    }
}


function tn_remove(me) {
    tn_remove_child(me.Parent, me.Name);
}

function tn_remove_child_maybe(me, x) {
    if(typeof x === "string") {
        console.assert(me instanceof Tree_Node);

        let name = x;
        if(!(name in me)) {
            return false;
        }

        // NOTE(llw): Remove from dom. (Parent may not be me.Dom!)
        let dom = me[name].Dom;
        dom.parentNode.removeChild(dom);

        // NOTE(llw): Remove from tree.
        delete me[name];

        return true;
    }
    else if(x instanceof Tree_Node) {
        return tn_remove_child_maybe(me, x.Name);
    }

    console.assert(false);
    return false;
}

function tn_remove_child(me, x) {
    let removed = tn_remove_child_maybe(me, x);
    console.assert(removed);
}


function tn_add_wrapper(me, name) {
    console.assert(me instanceof Tree_Node);
    console.assert(typeof name === "string");

    let div = document.createElement("div");
    div.id = me.Dom.id + "-" + name;

    let node = new Tree_Node(me, div, name, "TBD");
    me.Dom.append(div);

    return node;
}

