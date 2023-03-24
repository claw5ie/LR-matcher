template<typename T>
bool
operator<(View<T> v0, View<T> v1)
{
  for (size_t i = 0; i < v0.count && i < v1.count; i++)
    if (v0.data[i] != v1.data[i])
      return v0.data[i] < v1.data[i];

  return v0.count < v1.count;
}

string
view_to_string(View<char> v)
{
  string result;
  result.resize(v.count);
  memcpy(&result[0], v.data, v.count);
  return result;
}

ostream &
operator<<(ostream &stream, View<char> v)
{
  while (v.count-- > 0)
    stream << *v.data++;
  return stream;
}
