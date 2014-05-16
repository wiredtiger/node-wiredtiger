{
  "targets": [
    {
      "target_name": "wiredtiger",
      "sources": [
          "src/wiredtiger.cc"
      ],
      "include_dirs": [ "./include"],
      "link_settings": {
        "libraries": [ "../lib/libwiredtiger.a" ]
      }
    }
  ]
}
