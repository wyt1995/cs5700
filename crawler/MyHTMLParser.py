from html.parser import HTMLParser

class MyHTMLParser(HTMLParser):
    def __init__(self):
        super().__init__()
        self.links = []
        self.flags = []
        self.flag_found = False
        self.csrf_token = None

    def handle_starttag(self, tag, attrs):
        if tag == "a":
            for name, value in attrs:
                if name == "href" and "/fakebook" in value:
                    self.links.append(value)
        elif tag == "input":
            attributes = dict(attrs)
            if attributes.get("type") == "hidden" and attributes.get("name") == "csrfmiddlewaretoken":
                self.csrf_token = attributes.get("value")
        elif tag == "h3":
            attributes = dict(attrs)
            if attributes.get("class") == "secret_flag":
                self.flag_found = True

    def handle_endtag(self, tag):
        if tag == "h3" and self.flag_found:
            self.flag_found = False

    def handle_data(self, data):
        if self.flag_found:
            if data.strip().startswith("FLAG"):
                flag = data.strip().split("FLAG:")[1].strip()
                self.flags.append(flag)

    def clear(self):
        self.links.clear()
        self.flags.clear()
