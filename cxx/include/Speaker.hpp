#ifndef SPEAKER_H
#define SPEAKER_H

#include<string>
#include<unordered_map>

class Speaker {
public:
  Speaker() = default;
  Speaker(const std::string& name);
  Speaker(const Speaker&) = default;
  ~Speaker() = default;
  Speaker& operator=(const Speaker&) = default;

  bool operator==(const Speaker& speaker) const;
  bool operator<(const Speaker& speaker) const;

  bool empty() const;
  const std::string& name() const;
  unsigned citations() const;

  static void read(const std::string& filename);
private:
  std::string name_;
  static std::unordered_map<std::string, unsigned> citation_map_;
};

#endif /* SPEAKER_H */