{
  "targets": [
    {
      "target_name": "wiredtiger",
      "sources": [
          "src/wiredtiger.cc",
          "src/AsyncWorkers.cc",
          "src/WTConnection.cc",
          "src/WTCursor.cc",
          "src/WTTable.cc"
      ],
      "include_dirs": [ "./include", "<!(node -e \"require('nan')\")" ],
      "link_settings": {
        "libraries": [ "../lib/libwiredtiger.a" ]
      }
    }
  ]
}
