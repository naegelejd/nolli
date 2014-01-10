import stuff, junk

struct Event {
    str name
}

iface Eventer {
    func fire ()
}

func @Event fire () {
    print("event fired")
}

Event click = { "click" }
Event scroll = { "scroll" }

map{Event, func int (str)} eventHandlers = {
    click:handleClick,
    scroll:handleScroll
}

func int handleClick (str name) {
    print(name)
    return 1
}

func int handleScroll (str name) {
    print(name)
    return 2
}

func getHandler(Event event) {
    if eventHandlers.contains(event) {
        return eventHandlers[event]
    }

    return func int (str s) {
        print(s)
        return 0
    }
}

func main () = {
    func int (str) handle = getHandlers(click)
    handle(click.name)
}

main()
