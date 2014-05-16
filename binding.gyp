{
  "targets": [
    {
      "target_name": "wiredtiger",
      "sources": [
          "src/wiredtiger.cc",
          "src/wiredtiger_async.cc",
          "src/WTConnection.cc"
      ],
      "include_dirs": [ "./include", "<!(node -e \"require('nan')\")" ],
      "link_settings": {
        "libraries": [ "../lib/libwiredtiger.a" ]
      }
    }
  ]
}
